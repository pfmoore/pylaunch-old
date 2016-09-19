param(
$ver,
[scriptblock]$cmd
)

$key = "\Software\Python\PythonCore\$ver\InstallPath"
$path = (Get-ItemProperty -ea 0 HKCU:$key)."(Default)"
if ($path -eq $null) {
    $path = (Get-ItemProperty -ea 0 HKLM:$key)."(Default)"
}
if ($path -eq $null) {
    throw "Python version $ver not installed"
}

$p = $env:PATH
try {
    $env:PATH = "$path;$env:PATH"
    & $cmd
} finally {
    $env:PATH = $p
}
