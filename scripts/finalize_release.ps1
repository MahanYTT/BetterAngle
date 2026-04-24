# Automated Release Finalizer for BetterAngle
$version = (Get-Content VERSION).Trim()
$tag = "v$version"
$targetFile = "bin/BetterAngle_Setup.exe"

Write-Host "Finalizing Release $tag..."

# 1. Wait for Inno Setup to finish (usually runs after MSBuild in the workflow)
$maxWait = 300 # 5 minutes
$waited = 0
while (!(Test-Path $targetFile) -and $waited -lt $maxWait) {
    Start-Sleep -Seconds 10
    $waited += 10
    Write-Host "Waiting for installer ($waited/300s)..."
}

if (Test-Path $targetFile) {
    Write-Host "Installer found. Creating GitHub Release..."
    # Check if release already exists to avoid errors
    $exists = gh release view $tag 2>$null
    if ($null -eq $exists) {
        gh release create $tag $targetFile --title "BetterAngle Pro $version" --notes "Automated release triggered by CI build." --draft=false
        Write-Host "Release $tag created successfully."
    } else {
        Write-Host "Release $tag already exists. Uploading artifact..."
        gh release upload $tag $targetFile --clobber
    }
} else {
    Write-Host "ERROR: Installer not found after 5 minutes."
    exit 1
}
