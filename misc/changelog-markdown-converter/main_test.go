package main

import (
	"strings"
	"testing"

	"changelog-markdown-converter/converter"
)

func TestConversion1(t *testing.T) {
	in := "# Header1\n## Header2\n### Header3\n#### Header4\nSome text  \nwith many lines\n"
	want := "^uHEADER1\n\n^uHeader2\n\n^dHeader3\n\n^fHeader4\n\nSome text\nwith many lines\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestConversion2(t *testing.T) {
	in := "# HEADER\n\nSome **bold** text and some `fenced text` which is very cool.\n"
	want := "^uHEADER\n\nSome ^ibold^9 text and some ^8fenced text^9 which is very cool.\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestConversion3(t *testing.T) {
	in := "Some cool:\n```\nMulti line\nfenced text\n```\nwhich is really neat!\n"
	want := "Some cool:\n\n  ^8Multi line\n  ^8fenced text\n\nwhich is really neat!\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestConversion4(t *testing.T) {
	in := "* foo\n  * bar\n    * baz\n"
	want := "* foo\n  * bar\n    * baz\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestUnspecifiedCharactersRemainUntouched(t *testing.T) {
	in := "Plain text with _no_ configured conversion.\n"
	want := in

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestSmartLineBreakingKeepsIndentAndWidth(t *testing.T) {
	in := "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa bbbbbbbbbb\n"
	want := "    aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n    bbbbbbbbbb\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}

	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}

	for _, line := range strings.Split(strings.TrimSuffix(string(got), "\n"), "\n") {
		if len([]rune(line)) > 85 {
			t.Fatalf("line exceeds 85 chars: %q", line)
		}
	}
}

func TestSmartLineWrappingSpecExample(t *testing.T) {
	in := "some very long line which should be wrapped eventually namely soon right here yes sir yes sir yes sir yes sir\n\n* some very long line which should be wrapped eventually namely soon right here yes sir yes sir yes sir yes sir\n"
	want := "some very long line which should be wrapped eventually namely soon right here yes sir\nyes sir yes sir yes sir\n\n* some very long line which should be wrapped eventually namely soon right here yes\n  sir yes sir yes sir yes sir\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}

	for _, line := range strings.Split(strings.TrimSuffix(string(got), "\n"), "\n") {
		if len([]rune(line)) > 85 {
			t.Fatalf("line exceeds 85 chars: %q", line)
		}
	}
}

func TestSmartLineWrappingHonorsNumerationIndent(t *testing.T) {
	in := "1. some very long line which should be wrapped eventually namely soon right here yes sir yes sir yes sir yes sir\n"
	want := "1. some very long line which should be wrapped eventually namely soon right here yes\n   sir yes sir yes sir yes sir\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestGapsExampleFromSpec(t *testing.T) {
	in := "#### Unix\n* Implemented `Sys_Microseconds` function\n#### Windows\n### Client\n* Fixed `qwfwd` (QuakeWorld Forward) proxy compatibility which is a recommended proxy for finding lower ping connection to QuakeWorld servers\n* Fixed demo timeout when rewinding\n* Fixed gamestate not updating on demo rewind in some cases\n* Allowed rewinding/fast-forwarding when demo is paused\n"
	want := "^fUnix\n\n* Implemented ^8Sys_Microseconds^9 function\n\n^fWindows\n\n^dClient\n\n* Fixed ^8qwfwd^9 (QuakeWorld Forward) proxy compatibility which is a recommended\n  proxy for finding lower ping connection to QuakeWorld servers\n* Fixed demo timeout when rewinding\n* Fixed gamestate not updating on demo rewind in some cases\n* Allowed rewinding/fast-forwarding when demo is paused\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}
}

func TestFencedBlockWrapKeepsCodePrefixOnWrappedLines(t *testing.T) {
	in := "Intro:\n```\nThis is a very long fenced line that should wrap and keep the code color marker active on wrapped output lines too.\n```\nOutro.\n"
	want := "Intro:\n\n  ^8This is a very long fenced line that should wrap and keep the code color marker\n  ^8active on wrapped output lines too.\n\nOutro.\n"

	got, err := converter.ConvertMarkdownToChangelog([]byte(in))
	if err != nil {
		t.Fatalf("convert failed: %v", err)
	}
	if string(got) != want {
		t.Fatalf("unexpected output\nwant:\n%q\n\ngot:\n%q", want, string(got))
	}

	lines := strings.Split(strings.TrimSuffix(string(got), "\n"), "\n")
	for _, line := range lines {
		if len([]rune(line)) > 85 {
			t.Fatalf("line exceeds 85 chars: %q", line)
		}
	}
}
