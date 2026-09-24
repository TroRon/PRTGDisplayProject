param(
 [Parameter(Mandatory=$true)][string]$HostName,
 [Parameter(Mandatory=$true)][string]$Node,
 [Parameter(Mandatory=$true)][string]$TokenID,
 [Parameter(Mandatory=$true)][string]$SecretFile,
 [string]$GuestTag='monitor',
 [int]$Port=8006
)
$ErrorActionPreference='Stop'
if($HostName -notmatch '^[a-zA-Z0-9.-]+$' -or $Port -lt 1 -or $Port -gt 65535){throw 'Invalid host or port'}
$Base="https://${HostName}:$Port/api2/json"
function R($c,$v,$u='Count',$f=$false){$x=[ordered]@{channel=$c;value=$v;unit=$u};if($f){$x.float=1;$x.decimalmode='Auto'};$x}
try {
 $s=[IO.File]::ReadAllText($SecretFile).Trim(); $h=@{Authorization="PVEAPIToken=$TokenID=$s"}
 $nodes=(Invoke-RestMethod "$Base/nodes" -Headers $h -TimeoutSec 10).data; $n=$nodes|? node -eq $Node|select -First 1; if(-not$n){throw "Node $Node nicht gefunden"}
 $resources=(Invoke-RestMethod "$Base/cluster/resources" -Headers $h -TimeoutSec 10).data
 $guests=@($resources|?{($_.type -in @('qemu','lxc')) -and $_.node -eq $Node -and (($_.tags -split ';') -contains $GuestTag)}); $running=@($guests|? status -eq 'running').Count; $down=$guests.Count-$running
 $stor=@($resources|?{$_.type -eq 'storage' -and $_.node -eq $Node}); $storageProblems=@($stor|?{$_.status -and $_.status -ne 'available'}).Count
 $clusterNodes=@($nodes).Count; $clusterOnline=@($nodes|? status -eq 'online').Count
 $status=Invoke-RestMethod "$Base/cluster/status" -Headers $h -TimeoutSec 10; $quorate=if(@($status.data|? type -eq 'cluster')[0].quorate){1}else{0}
 $cpu=[math]::Round([double]$n.cpu*100,1); $ram=[math]::Round(([double]$n.mem/[double]$n.maxmem)*100,1); $disk=[math]::Round(([double]$n.disk/[double]$n.maxdisk)*100,1); $up=[math]::Round([double]$n.uptime/86400,2)
 $res=@((R 'Node Online' $(if($n.status-eq'online'){1}else{0})),(R 'CPU Usage' $cpu 'Percent' $true),(R 'RAM Usage' $ram 'Percent' $true),(R 'Root Disk Usage' $disk 'Percent' $true),(R 'Uptime Days' $up 'Custom' $true),(R 'Main Guests' $guests.Count),(R 'Main Guests Running' $running),(R 'Main Guests Down' $down),(R 'Storage Problems' $storageProblems),(R 'Cluster Nodes' $clusterNodes),(R 'Cluster Nodes Online' $clusterOnline),(R 'Cluster Quorum' $quorate))
 [ordered]@{prtg=[ordered]@{result=$res;text="$Node OK | CPU $cpu% | RAM $ram% | Main $running/$($guests.Count)"}}|ConvertTo-Json -Depth 8 -Compress
}catch{[ordered]@{prtg=[ordered]@{error=1;text="Proxmox-Abfrage fehlgeschlagen; Konfiguration, TLS und Leserechte prüfen."}}|ConvertTo-Json -Depth 5 -Compress}
