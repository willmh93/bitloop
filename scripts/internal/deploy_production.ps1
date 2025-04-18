# Variables
$source = "$PSScriptRoot\..\..\deployed\web\*"
$destination = "root@bitloop.dev:/var/www/bitloop-site/main/"
$key = "C:\Users\Will\bitloop_key"

Write-Host "Deploying files from: $source"
Write-Host "To: $destination`n"

# Run SCP
scp -i $key -o StrictHostKeyChecking=no $source $destination

# Pause
Write-Host "`nDeployment complete. Press Enter to continue..."
[void][System.Console]::ReadLine()
