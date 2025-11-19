<#
SPDX-FileCopyrightText: DagIR Contributors
SPDX-License-Identifier: MIT

Processes all `.expr` files in `samples\expressions` and generates
outputs for both `expression2tree` and `expression2bdd`.

Creates these folders under the repo root `samples`:
- `expression_tree_dot`     : DOT output from `expression2tree`
- `expression_tree_json`    : JSON output from `expression2tree`
- `expression_tree_mermaid` : Mermaid output from `expression2tree`
- `expression_bdd_dot`      : DOT output from `expression2bdd`
- `expression_bdd_json`     : JSON output from `expression2bdd`
- `expression_bdd_mermaid`  : Mermaid output from `expression2bdd`

Usage:
    pwsh.exe -ExecutionPolicy Bypass -File .\scripts\process_expressions.ps1

Optional parameters:
    -TreeExePath <path>   : Explicit path to `expression2tree.exe`.
    -BddExePath <path>    : Explicit path to `expression2bdd.exe`.
    -GraphvizPath <path>  : Explicit path to Graphviz `dot.exe`.
    -Library <teddy|cudd> : Library for expression2bdd (default: teddy).
    -StripMermaidFences   : Remove surrounding ```mermaid fences from mermaid output.
    -Verbose              : Show progress messages.

#>

param(
    [string]$TreeExePath = "",
    [string]$BddExePath = "",
    [string]$GraphvizPath = "",
    [ValidateSet('teddy','cudd')]
    [string]$Library = 'teddy',
    [switch]$StripMermaidFences,
    [switch]$Verbose,
    [int]$MaxDotSizeKB = 200
)

Set-StrictMode -Version Latest

$scriptRoot = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
$repoRoot = Resolve-Path (Join-Path $scriptRoot "..")

$exprDir = Join-Path $repoRoot "samples\expressions"
if (-not (Test-Path $exprDir)) {
    Write-Error "Expressions directory not found: $exprDir"
    exit 2
}

$out = [ordered]@{
    tree_dot     = Join-Path $repoRoot "samples\expression_tree_dot"
    tree_json    = Join-Path $repoRoot "samples\expression_tree_json"
    tree_mermaid = Join-Path $repoRoot "samples\expression_tree_mermaid"
    bdd_dot      = Join-Path $repoRoot "samples\expression_bdd_dot"
    bdd_json     = Join-Path $repoRoot "samples\expression_bdd_json"
    bdd_mermaid  = Join-Path $repoRoot "samples\expression_bdd_mermaid"
}

foreach ($d in $out.Values) {
    if (-not (Test-Path $d)) {
        New-Item -ItemType Directory -Path $d | Out-Null
        if ($Verbose) { Write-Host "Created: $d" }
    }
}

function Find-Exe {
    param(
        [string]$provided,
        [string]$name
    )
    if ($provided -and (Test-Path $provided)) {
        return (Get-Item $provided)
    }

    $buildRoot = Join-Path $repoRoot 'build'
    if (-not (Test-Path $buildRoot)) { return $null }

    $found = Get-ChildItem -Path $buildRoot -Filter "$name.exe" -Recurse -ErrorAction SilentlyContinue |
             Sort-Object FullName |
             Select-Object -First 1
    return $found
}

$treeExe = Find-Exe -provided $TreeExePath -name 'expression2tree'
if (-not $treeExe) { Write-Warning "expression2tree.exe not found in build tree; pass -TreeExePath to specify it." }
elseif ($Verbose) { Write-Host "Using expression2tree: $($treeExe.FullName)" }

$bddExe = Find-Exe -provided $BddExePath -name 'expression2bdd'
if (-not $bddExe) { Write-Warning "expression2bdd.exe not found in build tree; pass -BddExePath to specify it." }
elseif ($Verbose) { Write-Host "Using expression2bdd: $($bddExe.FullName)" }

# Detect Graphviz 'dot' executable (use explicit path first, then PATH, then common locations)
$dotExe = $null
if ($GraphvizPath -and (Test-Path $GraphvizPath)) {
    $dotExe = Get-Item $GraphvizPath
} else {
    $dotCmd = Get-Command dot -ErrorAction SilentlyContinue
    if ($dotCmd) {
        try { $dotExe = Get-Item $dotCmd.Path } catch { $dotExe = $null }
    }
    if (-not $dotExe) {
        $cands = @(
            "$env:ProgramFiles\Graphviz\bin\dot.exe",
            "$env:ProgramFiles(x86)\Graphviz\bin\dot.exe"
        )
        foreach ($p in $cands) {
            if (Test-Path $p) { $dotExe = Get-Item $p; break }
        }
    }
}
if ($dotExe) {
    if ($Verbose) { Write-Host "Graphviz dot found: $($dotExe.FullName)" }
} else {
    if ($Verbose) { Write-Host "Graphviz dot not found; PNG generation disabled." }
}

function Run-And-Save {
    param(
        [System.IO.FileInfo]$exe,
        [string[]]$arguments,
        [string]$outfile,
        [switch]$stripMermaid
    )

    if (-not $exe) {
        if ($arguments) { $argText = $arguments -join ' ' } else { $argText = '' }
        Write-Warning "Skipping (exe not available): $argText"
        return
    }

    $cmd = @($exe.FullName) + ($arguments -ne $null ? $arguments : @())
    if ($Verbose) { Write-Host "Running: $($cmd -join ' ')" }

    $proc = & $exe.FullName @arguments 2>&1
    $exit = $LASTEXITCODE

    if ($exit -ne 0) {
        $argText = $arguments -join ' '
        Write-Warning "Process failed (exit $exit): $($exe.Name) with args $argText"
    }

    $outText = $proc -join "`n"

    if ($stripMermaid) {
        $outText = $outText -replace '^\s*```mermaid\s*','' -replace '\s*```\s*$',''
    }

    $outDir = Split-Path -Parent $outfile
    if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

    $outText | Out-File -FilePath $outfile -Encoding utf8
    if ($Verbose) { Write-Host "Wrote: $outfile" }
}

$exprFiles = Get-ChildItem -Path $exprDir -Filter '*.expr' -File | Sort-Object Name
foreach ($f in $exprFiles) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($f.Name)

    # expression2tree outputs
    if ($treeExe) {
        $dotOut = Join-Path $out.tree_dot "$base.dot"
        Run-And-Save -exe $treeExe -arguments @($f.FullName,'dot') -outfile $dotOut
        if ($dotExe -and (Test-Path $dotOut)) {
            $pngOut = [System.IO.Path]::ChangeExtension($dotOut, '.png')
            $dotInfo = Get-Item $dotOut
            $sizeKB = [math]::Ceiling($dotInfo.Length / 1KB)
            if ($sizeKB -gt $MaxDotSizeKB) {
                if ($Verbose) { Write-Host "Skipping DOT->PNG for $dotOut (size ${sizeKB}KB > ${MaxDotSizeKB}KB)" }
            } else {
                if ($Verbose) { Write-Host "Converting DOT -> PNG: $dotOut -> $pngOut" }
                $convOut = & $dotExe.FullName -Tpng -o $pngOut $dotOut 2>&1
                $convExit = $LASTEXITCODE
                if ($convExit -ne 0) {
                    $msg = $convOut -join "`n"
                    Write-Warning ("Graphviz conversion failed (exit {0}) for {1}: {2}" -f $convExit, $dotOut, $msg)
                }
                elseif ($Verbose) { Write-Host "Wrote: $pngOut" }
            }
        }

        $jsonOut = Join-Path $out.tree_json "$base.json"
        Run-And-Save -exe $treeExe -arguments @($f.FullName,'json') -outfile $jsonOut

        $mmdExt = if ($StripMermaidFences) { '.mmd' } else { '.md' }
        $mmdOut = Join-Path $out.tree_mermaid "$base$mmdExt"
        Run-And-Save -exe $treeExe -arguments @($f.FullName,'mermaid') -outfile $mmdOut -stripMermaid:$StripMermaidFences
    }

    # expression2bdd outputs (use provided Library)
    if ($bddExe) {
        $dotOut = Join-Path $out.bdd_dot "$base.dot"
        Run-And-Save -exe $bddExe -arguments @($f.FullName,$Library,'dot') -outfile $dotOut
        if ($dotExe -and (Test-Path $dotOut)) {
            $pngOut = [System.IO.Path]::ChangeExtension($dotOut, '.png')
            $dotInfo = Get-Item $dotOut
            $sizeKB = [math]::Ceiling($dotInfo.Length / 1KB)
            if ($sizeKB -gt $MaxDotSizeKB) {
                if ($Verbose) { Write-Host "Skipping DOT->PNG for $dotOut (size ${sizeKB}KB > ${MaxDotSizeKB}KB)" }
            } else {
                if ($Verbose) { Write-Host "Converting DOT -> PNG: $dotOut -> $pngOut" }
                $convOut = & $dotExe.FullName -Tpng -o $pngOut $dotOut 2>&1
                $convExit = $LASTEXITCODE
                if ($convExit -ne 0) {
                    $msg = $convOut -join "`n"
                    Write-Warning ("Graphviz conversion failed (exit {0}) for {1}: {2}" -f $convExit, $dotOut, $msg)
                }
                elseif ($Verbose) { Write-Host "Wrote: $pngOut" }
            }
        }

        $jsonOut = Join-Path $out.bdd_json "$base.json"
        Run-And-Save -exe $bddExe -arguments @($f.FullName,$Library,'json') -outfile $jsonOut

        $mmdExt = if ($StripMermaidFences) { '.mmd' } else { '.md' }
        $mmdOut = Join-Path $out.bdd_mermaid "$base$mmdExt"
        Run-And-Save -exe $bddExe -arguments @($f.FullName,$Library,'mermaid') -outfile $mmdOut -stripMermaid:$StripMermaidFences
    }
}

Write-Host "Done. Generated outputs in:"
foreach ($d in $out.GetEnumerator()) { Write-Host " - $($d.Value)" }
