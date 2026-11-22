[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PythonExecutable,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeDirectory
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$requirementsFile = Join-Path $repositoryRoot "requirements-research.txt"

if (-not (Test-Path -LiteralPath $PythonExecutable -PathType Leaf)) {
    throw "Python executable was not found: $PythonExecutable"
}
if (-not (Test-Path -LiteralPath $requirementsFile -PathType Leaf)) {
    throw "Research requirements file was not found: $requirementsFile"
}

$python = (Resolve-Path -LiteralPath $PythonExecutable).Path
& $python -c "import sys; raise SystemExit(0 if sys.version_info >= (3, 12) else 1)"
if ($LASTEXITCODE -ne 0) {
    throw "Python 3.12 or newer is required."
}
$runtime = [System.IO.Path]::GetFullPath($RuntimeDirectory)
if (Test-Path -LiteralPath $runtime) {
    throw "Refusing to overwrite an existing runtime directory: $runtime"
}

$parent = Split-Path -Parent $runtime
if (-not (Test-Path -LiteralPath $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}

Write-Host "Creating an isolated research Python environment..."
& $python -m venv $runtime
if ($LASTEXITCODE -ne 0) {
    throw "Python virtual environment creation failed with exit code $LASTEXITCODE."
}

$venvPython = Join-Path $runtime "Scripts\python.exe"
if (-not (Test-Path -LiteralPath $venvPython -PathType Leaf)) {
    throw "The virtual environment Python executable was not created: $venvPython"
}

Write-Host "Installing the research-only Python dependencies..."
& $venvPython -m pip install --upgrade pip
if ($LASTEXITCODE -ne 0) {
    throw "pip upgrade failed with exit code $LASTEXITCODE."
}
& $venvPython -m pip install -r $requirementsFile
if ($LASTEXITCODE -ne 0) {
    throw "Research dependency installation failed with exit code $LASTEXITCODE."
}

Write-Host "Checking core Python imports..."
& $venvPython -c "import google.genai, openai, fitz, pymupdf4llm, numpy, matplotlib, pypdf"
if ($LASTEXITCODE -ne 0) {
    throw "Core research Python imports failed."
}

$missingLatex = @("pdflatex", "bibtex") | Where-Object {
    -not (Get-Command $_ -ErrorAction SilentlyContinue)
}
if ($missingLatex.Count -gt 0) {
    Write-Warning ("Python environment is ready, but LaTeX tools are missing from PATH: " +
        ($missingLatex -join ", ") +
        ". Install a LaTeX distribution and reopen the application before running the research engine.")
    exit 3
}

Write-Host "Research runtime is ready."
Write-Host "Select this Python executable in the application:"
Write-Host $venvPython
Write-Host "API credentials are not configured or stored by this script."
