#!/usr/bin/env bash
# Explicit local setup. No remote operations and no existing secret replacement.
set -euo pipefail
[[ $# == 2 && $EUID == 0 ]] || { echo 'Usage: sudo bash prepare-storage.sh DATA_DIRECTORY ADMIN_USER'; exit 2; }
for tool in realpath openssl setfacl runuser setpriv; do command -v "$tool" >/dev/null; done
root=$(realpath -m -- "$1"); admin_uid=$(id -u -- "$2")
[[ "$root" != / && ! -L "$1" ]] || exit 2
if [[ -d "$root" ]]; then
 [[ -z $(find "$root" ! -type d ! -type f -print -quit) && -z $(find "$root" -type f -links +1 -print -quit) ]] || { echo 'Linked/special files found; inspect directory first.'; exit 1; }
fi
install -d -m 0750 -o root -g 1000 -- "$root" "$root/config" "$root/secrets"
install -d -m 0750 -o 1000 -g 1000 -- "$root/firmware" "$root/admin"
here=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ ! -e "$root/config/aggregator.json" ]]; then install -m 0640 -o root -g 1000 -- "$here/config/aggregator.example.json" "$root/config/aggregator.json"; fi
umask 027
for kind in panel_api_token ota_admin_token; do
 if [[ ! -e "$root/secrets/$kind.txt" ]]; then openssl rand -hex 32 > "$root/secrets/$kind.txt"; fi
done
if [[ ! -e "$root/secrets/prtg_api_token.txt" ]]; then
 read -r -s -p 'PRTG API token (hidden; Enter to configure later in WebAdmin): ' task_prtg; printf '\n'
 printf '%s\n' "$task_prtg" > "$root/secrets/prtg_api_token.txt"; unset task_prtg
fi
for file in "$root"/secrets/*.txt; do chown root:1000 "$file"; chmod 0640 "$file"; done
setfacl -R -P -m "u:$admin_uid:rwX" -- "$root"
find "$root" -type d -exec setfacl -m "d:u:$admin_uid:rwx" -- {} +
runuser -u "$2" -- test -w "$root/firmware"
runuser -u "$2" -- test -r "$root/secrets/panel_api_token.txt"
echo 'Local storage prepared. Edit config/aggregator.json before starting. No token printed.'

for directory in "$root/admin" "$root/firmware"; do
 probe=$(setpriv --reuid 1000 --regid 1000 --clear-groups mktemp "$directory/.acl-test.XXXXXX")
 chmod 0660 "$probe"
 runuser -u "$2" -- test -r "$probe"
 runuser -u "$2" -- test -w "$probe"
 rm -f -- "$probe"
done
echo 'WebAdmin: configure ADMIN_ORIGIN, then open /admin with the separate OTA administrator token.'
