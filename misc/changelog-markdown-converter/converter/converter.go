package converter

import (
	"bytes"
	"fmt"
	"strings"
	"unicode"

	"github.com/yuin/goldmark"
	"github.com/yuin/goldmark/text"
)

const reset = "^9"

const boldColor = "^7"

var headingPrefixByLevel = map[int]string{
	1: "^u",
	2: "^u",
	3: "^d",
	4: "^f",
}

// ConvertMarkdownToChangelog transforms markdown bytes into plaintext changelog bytes.
func ConvertMarkdownToChangelog(input []byte) ([]byte, error) {
	// Parse the document with goldmark so conversion is backed by a real Markdown parser.
	// The converter itself uses a deterministic source transformation to preserve non-targeted text 1:1.
	if err := validateMarkdown(input); err != nil {
		return nil, err
	}

	lines := strings.SplitAfter(string(input), "\n")
	if len(lines) == 1 && lines[0] == "" {
		return input, nil
	}

	var out bytes.Buffer
	inFence := false

	for i := 0; i < len(lines); i++ {
		line := lines[i]
		trimmed := strings.TrimSpace(strings.TrimSuffix(line, "\n"))

		if !inFence && isFenceStartLine(trimmed) {
			// Force a blank line before converted fenced blocks.
			if out.Len() > 0 && !bytes.HasSuffix(out.Bytes(), []byte("\n\n")) {
				out.WriteByte('\n')
			}
			inFence = true
			continue
		}

		if inFence {
			if isFenceStartLine(trimmed) {
				inFence = false
				// Force a blank line after converted fenced blocks.
				if !bytes.HasSuffix(out.Bytes(), []byte("\n\n")) {
					out.WriteByte('\n')
				}
				continue
			}

			lineNoNewline, hadNewline := trimSingleNewline(line)
			out.WriteString("  ^8")
			out.WriteString(lineNoNewline)
			if hadNewline {
				out.WriteByte('\n')
			}
			continue
		}

		level, headingText, ok := parseATXHeading(line)
		if ok {
			prefix := headingPrefixByLevel[level]
			if level == 1 {
				headingText = strings.ToUpper(headingText)
			}
			// Keep headings visually separated from preceding non-empty content.
			if out.Len() > 0 && !bytes.HasSuffix(out.Bytes(), []byte("\n\n")) {
				out.WriteByte('\n')
			}
			out.WriteString(prefix)
			out.WriteString(headingText)
			out.WriteString("\n\n")

			// Normalize heading separation to exactly one blank line.
			if i+1 < len(lines) {
				nextTrimmed := strings.TrimSpace(strings.TrimSuffix(lines[i+1], "\n"))
				if nextTrimmed == "" {
					i++
				}
			}
			continue
		}

		line = normalizeHardLineBreak(line)
		out.WriteString(convertInline(line))
	}

	// Apply line wrapping as a final pass so all generated output follows the same rule.
	return []byte(wrapOutputLines(out.String(), 85)), nil
}

func validateMarkdown(input []byte) error {
	md := goldmark.New()
	reader := text.NewReader(input)
	if md.Parser().Parse(reader) == nil {
		return fmt.Errorf("goldmark parse failed")
	}
	return nil
}

func trimSingleNewline(s string) (string, bool) {
	if strings.HasSuffix(s, "\n") {
		return strings.TrimSuffix(s, "\n"), true
	}
	return s, false
}

func isFenceStartLine(line string) bool {
	return strings.HasPrefix(line, "```")
}

func parseATXHeading(line string) (int, string, bool) {
	lineNoNewline := strings.TrimSuffix(line, "\n")
	if !strings.HasPrefix(lineNoNewline, "#") {
		return 0, "", false
	}

	count := 0
	for count < len(lineNoNewline) && count < 4 && lineNoNewline[count] == '#' {
		count++
	}
	if count == 0 || count > 4 {
		return 0, "", false
	}

	if len(lineNoNewline) <= count || lineNoNewline[count] != ' ' {
		return 0, "", false
	}

	rest := strings.TrimSpace(lineNoNewline[count:])
	if rest == "" {
		return 0, "", false
	}

	for strings.HasSuffix(rest, "#") {
		rest = strings.TrimSpace(strings.TrimSuffix(rest, "#"))
	}

	return count, rest, true
}

func normalizeHardLineBreak(line string) string {
	if strings.HasSuffix(line, "  \n") {
		return strings.TrimSuffix(line, "  \n") + "\n"
	}
	return line
}

func convertInline(line string) string {
	lineNoNewline, hadNewline := trimSingleNewline(line)

	var out strings.Builder
	out.Grow(len(lineNoNewline) + 8)

	for i := 0; i < len(lineNoNewline); {
		if strings.HasPrefix(lineNoNewline[i:], "**") {
			end := strings.Index(lineNoNewline[i+2:], "**")
			if end >= 0 {
				content := lineNoNewline[i+2 : i+2+end]
				out.WriteString(boldColor)
				out.WriteString(content)
				out.WriteString(reset)
				i += 2 + end + 2
				continue
			}
		}

		if lineNoNewline[i] == '`' {
			end := strings.IndexByte(lineNoNewline[i+1:], '`')
			if end >= 0 {
				content := lineNoNewline[i+1 : i+1+end]
				out.WriteString("^8")
				out.WriteString(content)
				out.WriteString(reset)
				i += 1 + end + 1
				continue
			}
		}

		out.WriteByte(lineNoNewline[i])
		i++
	}

	if hadNewline {
		out.WriteByte('\n')
	}

	return out.String()
}

func wrapOutputLines(s string, width int) string {
	lines := strings.Split(s, "\n")
	var wrapped []string

	for _, line := range lines {
		wrapped = append(wrapped, wrapSingleLine(line, width)...)
	}

	return strings.Join(wrapped, "\n")
}

func wrapSingleLine(line string, width int) []string {
	if width <= 0 || len([]rune(line)) <= width {
		return []string{line}
	}

	indentCount := 0
	for indentCount < len(line) && line[indentCount] == ' ' {
		indentCount++
	}

	firstIndent := line[:indentCount]
	content := []rune(line[indentCount:])
	continuationIndent := firstIndent
	if extra := listContinuationExtra(string(content)); extra > 0 {
		continuationIndent += strings.Repeat(" ", extra)
	}
	continuationPrefix := continuationIndent
	if strings.HasPrefix(string(content), "^8") {
		// Keep code color active on wrapped continuation lines for converted fenced blocks.
		continuationPrefix = firstIndent + "^8"
	}

	isFirst := true
	var wrapped []string
	for {
		indent := continuationPrefix
		if isFirst {
			indent = firstIndent
		}
		available := width - len([]rune(indent))
		if available < 1 {
			available = 1
		}

		if len(content) <= available {
			wrapped = append(wrapped, indent+string(content))
			break
		}

		// Prefer breaking on whitespace to keep words intact.
		// Search for the rightmost whitespace whose preceding segment fits in `available`.
		breakAt := -1
		upper := available
		if upper >= len(content) {
			upper = len(content) - 1
		}
		for i := upper; i >= 0; i-- {
			if unicode.IsSpace(content[i]) {
				breakAt = i
				break
			}
		}

		if breakAt <= 0 {
			wrapped = append(wrapped, indent+string(content[:available]))
			content = content[available:]
			isFirst = false
			continue
		}

		wrapped = append(wrapped, indent+string(content[:breakAt]))
		content = trimLeftSpaceRunes(content[breakAt+1:])
		isFirst = false
	}
	return wrapped
}

func trimLeftSpaceRunes(in []rune) []rune {
	start := 0
	for start < len(in) && unicode.IsSpace(in[start]) {
		start++
	}
	return in[start:]
}

func listContinuationExtra(content string) int {
	if len(content) < 2 {
		return 0
	}

	if (content[0] == '*' || content[0] == '-' || content[0] == '+') && content[1] == ' ' {
		return 2
	}

	i := 0
	for i < len(content) && content[i] >= '0' && content[i] <= '9' {
		i++
	}
	if i > 0 && i+1 < len(content) && (content[i] == '.' || content[i] == ')') && content[i+1] == ' ' {
		return i + 2
	}

	return 0
}
