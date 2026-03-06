//go:build ignore

package main

import (
	"bytes"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"strings"

	"changelog-markdown-converter/converter"
)

func main() {
	baseDir := "unittests"
	if len(os.Args) == 2 {
		baseDir = os.Args[1]
	}

	if err := runFixtureAssertions(baseDir); err != nil {
		fmt.Fprintf(os.Stderr, "fixture assertion failed: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("all markdown fixtures passed in %q\n", baseDir)
}

func runFixtureAssertions(baseDir string) error {
	found := 0

	err := filepath.WalkDir(baseDir, func(path string, d fs.DirEntry, walkErr error) error {
		if walkErr != nil {
			return walkErr
		}
		if d.IsDir() {
			return nil
		}
		if strings.ToLower(filepath.Ext(path)) != ".md" {
			return nil
		}

		found++
		mdBytes, err := os.ReadFile(path)
		if err != nil {
			return fmt.Errorf("read markdown %q: %w", path, err)
		}

		converted, err := converter.ConvertMarkdownToChangelog(mdBytes)
		if err != nil {
			return fmt.Errorf("convert markdown %q: %w", path, err)
		}

		goldPath := strings.TrimSuffix(path, filepath.Ext(path)) + ".gold"
		goldBytes, err := os.ReadFile(goldPath)
		if err != nil {
			if os.IsNotExist(err) {
				return fmt.Errorf("missing gold fixture for %q (expected %q)", path, goldPath)
			}
			return fmt.Errorf("read gold fixture %q: %w", goldPath, err)
		}

		if !bytes.Equal(converted, goldBytes) {
			return fmt.Errorf("mismatch for %q\n%s", path, unifiedLineDiff(goldBytes, converted))
		}

		return nil
	})
	if err != nil {
		return err
	}

	if found == 0 {
		return fmt.Errorf("no .md fixtures found in %q", baseDir)
	}

	return nil
}

func unifiedLineDiff(expected []byte, actual []byte) string {
	expectedLines := strings.Split(string(expected), "\n")
	actualLines := strings.Split(string(actual), "\n")

	var out strings.Builder
	out.WriteString("--- expected (.gold)\n")
	out.WriteString("+++ actual (converted)\n")

	maxLines := len(expectedLines)
	if len(actualLines) > maxLines {
		maxLines = len(actualLines)
	}

	for i := 0; i < maxLines; i++ {
		var expLine string
		var actLine string
		hasExp := i < len(expectedLines)
		hasAct := i < len(actualLines)
		if hasExp {
			expLine = expectedLines[i]
		}
		if hasAct {
			actLine = actualLines[i]
		}

		if hasExp && hasAct && expLine == actLine {
			continue
		}

		if hasExp {
			fmt.Fprintf(&out, "-%d:%s\n", i+1, expLine)
		}
		if hasAct {
			fmt.Fprintf(&out, "+%d:%s\n", i+1, actLine)
		}
	}

	return out.String()
}
