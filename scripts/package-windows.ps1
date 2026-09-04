param(
    [string]$BuildDirectory = "$env:LOCALAPPDATA/LAHelper/build/windows-release",
    [string]$OutputDirectory = "$PSScriptRoot/../dist/LAHelper"
)

$ErrorActionPreference = "Stop"
$build = [System.IO.Path]::GetFullPath($BuildDirectory)
$output = [System.IO.Path]::GetFullPath($OutputDirectory)
$release = Join-Path $build "Release"
$executable = Join-Path $release "LAHelper.exe"

if (-not (Test-Path $executable)) {
    throw "LAHelper.exe was not found. Run: cmake --build --preset windows-release"
}

if (Test-Path $output) {
    Remove-Item $output -Recurse -Force
}
New-Item $output -ItemType Directory | Out-Null
Copy-Item $executable $output

# vcpkg's app-local deployment places non-Qt dependencies beside the build executable.
Get-ChildItem $release -Filter "*.dll" | Copy-Item -Destination $output

$expectedDeployTool = Join-Path $build "vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe"
$deployTool = if (Test-Path $expectedDeployTool) { Get-Item $expectedDeployTool } else { $null }
if (-not $deployTool) {
    $deployTool = Get-ChildItem $build -Filter "windeployqt.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
}
if (-not $deployTool) {
    $deployTool = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
}
if (-not $deployTool) {
    throw "windeployqt.exe was not found. Add the Qt tools directory to PATH or use the vcpkg Qt installation."
}

$deployPath = if ($deployTool.Source) { $deployTool.Source } else { $deployTool.FullName }
& $deployPath --release --no-translations (Join-Path $output "LAHelper.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE."
}

Write-Host "Portable application created at: $output"
