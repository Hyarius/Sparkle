param (
    [Parameter(Mandatory=$true)]
    [string]$Folder
)

# Recursively find all C/C++ source/header files and apply clang-format in place
Get-ChildItem -Path $Folder -Recurse -Include *.cpp, *.hpp, *.c, *.h, *.cc, *.hh |
    ForEach-Object {
        clang-format -i $_.FullName
        Write-Output "Formatted: $($_.FullName)"
    }

Write-Output "Formatting applied to all source files in $Folder"
