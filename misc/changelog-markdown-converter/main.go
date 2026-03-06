package main

import (
	"changelog-markdown-converter/converter"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "usage: %s <input1.md> [input2.md ...]\n", os.Args[0])
		os.Exit(2)
	}

	for _, inputPath := range os.Args[1:] {
		// Only markdown files are accepted as explicit CLI inputs.
		if strings.ToLower(filepath.Ext(inputPath)) != ".md" {
			fmt.Fprintf(os.Stderr, "invalid input %q: expected .md extension\n", inputPath)
			os.Exit(2)
		}

		input, err := os.ReadFile(inputPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "read input %q: %v\n", inputPath, err)
			os.Exit(1)
		}

		output, err := converter.ConvertMarkdownToChangelog(input)
		if err != nil {
			fmt.Fprintf(os.Stderr, "convert %q: %v\n", inputPath, err)
			os.Exit(1)
		}

		outputPath := strings.TrimSuffix(inputPath, filepath.Ext(inputPath)) + ".changelog"
		if err := os.WriteFile(outputPath, output, 0o644); err != nil {
			fmt.Fprintf(os.Stderr, "write output %q: %v\n", outputPath, err)
			os.Exit(1)
		}
	}
}
