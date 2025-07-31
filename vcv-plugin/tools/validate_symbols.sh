#!/bin/bash
# Symbol validation script for VCV plugin refactoring
# Validates that all expected symbols are properly exported after refactoring

set -e

PLUGIN_DYLIB="plugin.dylib"
EXPECTED_SYMBOLS="expected_symbols.txt"

echo "🔍 Symbol Validation for VCV Plugin Refactoring"
echo "=============================================="

if [ ! -f "$PLUGIN_DYLIB" ]; then
    echo "❌ Error: $PLUGIN_DYLIB not found. Run 'make' first."
    exit 1
fi

echo "📋 Checking critical API symbols..."

# Check for C API functions that plugins depend on
CRITICAL_SYMBOLS=(
    "_vcv_drawText"
    "_NT_drawText"
    "_NT_drawShapeI"
    "_NT_drawShapeF"
    "__Z19getCurrentJsonParsev"
    "__Z20getCurrentJsonStreamv"
)

MISSING_SYMBOLS=()
FOUND_SYMBOLS=()

for symbol in "${CRITICAL_SYMBOLS[@]}"; do
    if nm "$PLUGIN_DYLIB" | grep -q "$symbol"; then
        FOUND_SYMBOLS+=("$symbol")
        echo "✅ $symbol"
    else
        MISSING_SYMBOLS+=("$symbol")
        echo "❌ $symbol - MISSING"
    fi
done

echo
echo "📊 Summary:"
echo "Found symbols: ${#FOUND_SYMBOLS[@]}"
echo "Missing symbols: ${#MISSING_SYMBOLS[@]}"

if [ ${#MISSING_SYMBOLS[@]} -gt 0 ]; then
    echo
    echo "❌ VALIDATION FAILED: Missing critical symbols"
    echo "This will cause plugin loading failures in VCV Rack."
    echo 
    echo "Missing symbols:"
    for symbol in "${MISSING_SYMBOLS[@]}"; do
        echo "  - $symbol"
    done
    echo
    echo "💡 To fix:"
    echo "1. Ensure functions are properly declared with correct linkage (extern \"C\" vs C++)"
    echo "2. Add global wrapper functions if moving to namespaces"
    echo "3. Check that all source files are included in the build"
    exit 1
else
    echo
    echo "✅ VALIDATION PASSED: All critical symbols found"
    echo "Plugin should load correctly in VCV Rack."
fi

echo
echo "🔍 Full symbol analysis:"
echo "Total exported symbols: $(nm "$PLUGIN_DYLIB" | wc -l)"
echo "Defined symbols (T): $(nm "$PLUGIN_DYLIB" | grep ' T ' | wc -l)"
echo "Undefined symbols (U): $(nm "$PLUGIN_DYLIB" | grep ' U ' | wc -l)"

echo
echo "🎯 Symbol validation complete!"