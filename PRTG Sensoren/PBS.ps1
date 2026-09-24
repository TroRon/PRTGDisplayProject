param(
 [Parameter(Mandatory=$true)][string]$BaseUri,
 [Parameter(Mandatory=$true)][string]$Datastore,
 [string]$Namespace='',
 [Parameter(Mandatory=$true)][string]$TokenID,
 [Parameter(Mandatory=$true)][string]$SecretFile,
 [ValidateRange(1,8760)][int]$MaxBackupAgeHours=30
)
$ErrorActionPreference='Stop'
$uri=[uri]$BaseUri
if($uri.Scheme -ne 'https' -or $uri.UserInfo -or $uri.Query -or $uri.Fragment){throw 'HTTPS API address without credentials required'}
$BaseUri=$BaseUri.TrimEnd('/')
$Datastore=[uri]::EscapeDataString($Datastore)
$Namespace=[uri]::EscapeDataString($Namespace)
function New-PrtgResult { param([string]$Channel,[double]$Value,[string]$Unit="Count",[bool]$Float=$false); $r=[ordered]@{channel=$Channel;value=$Value;unit=$Unit}; if($Float){$r.float=1;$r.decimalmode="Auto"}; return $r }
try {
 [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
 if(-not(Test-Path $SecretFile)){throw "PBS Token Secret nicht gefunden."}; $TokenSecret=[IO.File]::ReadAllText($SecretFile).Trim(); if([string]::IsNullOrWhiteSpace($TokenSecret)){throw "PBS Token Secret ist leer."}
 $Headers=@{Authorization="PBSAPIToken=$TokenID`:$TokenSecret"}
 $null=Invoke-RestMethod -Uri "$BaseUri/version" -Headers $Headers -TimeoutSec 15
 $Stores=Invoke-RestMethod -Uri "$BaseUri/status/datastore-usage" -Headers $Headers -TimeoutSec 15
 $GC=Invoke-RestMethod -Uri "$BaseUri/admin/datastore/$Datastore/gc" -Headers $Headers -TimeoutSec 15
 $Groups=Invoke-RestMethod -Uri "$BaseUri/admin/datastore/$Datastore/groups?ns=$Namespace" -Headers $Headers -TimeoutSec 15
 $Tasks=Invoke-RestMethod -Uri "$BaseUri/nodes/localhost/tasks?limit=100" -Headers $Headers -TimeoutSec 15
 $Store=$Stores.data|Where-Object{$_.store-eq$Datastore}|Select-Object -First 1; if(-not$Store){throw "Datastore '$Datastore' nicht gefunden."}
 $DatastoreOnline=if($Store.'mount-status'-eq'nonremovable'){1}else{0}; $Total=[double]$Store.total; $Used=[double]$Store.used; $Free=$Total-$Used; $UsagePercent=if($Total-gt0){[math]::Round(($Used/$Total)*100,1)}else{0}; $FreeTiB=[math]::Round($Free/[math]::Pow(1024,4),2)
 $GCHealth=if($GC.data.'last-run-state'-eq'OK'){1}else{0}; $NowUnix=[DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); $GCAgeHours=if($GC.data.'last-run-endtime'){[math]::Round(($NowUnix-[double]$GC.data.'last-run-endtime')/3600,1)}else{9999}; $GCPendingGiB=[math]::Round([double]$GC.data.'pending-bytes'/[math]::Pow(1024,3),2); $BadChunks=[int]$GC.data.'removed-bad'+[int]$GC.data.'still-bad'
 $BackupGroups=@($Groups.data); $BackupTotal=$BackupGroups.Count; $BackupCurrent=0; $BackupOld=0; $OldestAge=0
 foreach($Group in $BackupGroups){if(-not$Group.'last-backup'){$BackupOld++;continue};$AgeHours=($NowUnix-[double]$Group.'last-backup')/3600;if($AgeHours-gt$OldestAge){$OldestAge=$AgeHours};if($AgeHours-le$MaxBackupAgeHours){$BackupCurrent++}else{$BackupOld++}}
 $OldestAge=[math]::Round($OldestAge,1); $RecentBackupErrors=@($Tasks.data|Where-Object{$_.'worker_type'-eq'backup'-and$_.status-and$_.status-ne'OK'}).Count
 $Results=@((New-PrtgResult "PBS API Online" 1),(New-PrtgResult "Datastore Online" $DatastoreOnline),(New-PrtgResult "Datastore Usage" $UsagePercent "Percent" $true),(New-PrtgResult "Datastore Free" $FreeTiB "Custom" $true),(New-PrtgResult "GC Health" $GCHealth),(New-PrtgResult "GC Age Hours" $GCAgeHours "Custom" $true),(New-PrtgResult "GC Pending GiB" $GCPendingGiB "Custom" $true),(New-PrtgResult "Bad Chunks" $BadChunks),(New-PrtgResult "Daily Backups Total" $BackupTotal),(New-PrtgResult "Daily Backups Current" $BackupCurrent),(New-PrtgResult "Daily Backups Old" $BackupOld),(New-PrtgResult "Oldest Daily Backup Hours" $OldestAge "Custom" $true),(New-PrtgResult "Recent Backup Errors" $RecentBackupErrors))
 [ordered]@{prtg=[ordered]@{result=$Results;text="PBS OK | Store $UsagePercent% | Daily $BackupCurrent/$BackupTotal | GC $($GC.data.'last-run-state')"}}|ConvertTo-Json -Depth 10 -Compress
} catch {[ordered]@{prtg=[ordered]@{error=1;text="PBS-Abfrage fehlgeschlagen; Konfiguration, TLS und Leserechte prüfen."}}|ConvertTo-Json -Depth 5 -Compress} finally {Remove-Variable TokenSecret,Headers -ErrorAction SilentlyContinue}
