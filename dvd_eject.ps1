param(
    [Parameter()]
    [String]$drive
)
$shell = New-Object -ComObject Shell.Application
$dvd_drive = $shell.NameSpace($drive)
$dvd_drive.Self.InvokeVerb('Eject')
# $dvd_drive.Self.InvokeVerb('Close')
