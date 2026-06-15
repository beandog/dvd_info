$dvd_drive = Get-CimInstance -Class Win32_CDROMDrive | Select-Object -First 1

$drive_info = new-object psobject
$drive_info | Add-Member noteproperty drive ($dvd_drive.drive)
$drive_info | Add-Member noteproperty has_media ($dvd_drive.MediaLoaded)
$drive_info | Add-Member noteproperty status ($dvd_drive.Status)
$drive_info | Add-Member noteproperty filesize ($dvd_drive.Size)
$drive_info | Add-Member noteproperty volume_name ($dvd_drive.volumeName)
$drive_info | Add-Member noteproperty serial_number ($dvd_drive.VolumeSerialNumber)

Write-Output $drive_info | ConvertTo-Json
