# Install clang-format using pip
pip install clang-format==17.0.6

# Define the list of file extensions to format
$fileExtensions = @("*.c", "*.cpp", "*.cxx", "*.h", "*.hpp", "*.cu")

# Recursively find all files with the specified extensions
foreach ($extension in $fileExtensions) {
    Get-ChildItem -Recurse -File -Include $extension | ForEach-Object {
        # Format the file in-place using clang-format
        clang-format -i $_.FullName
    }
}
