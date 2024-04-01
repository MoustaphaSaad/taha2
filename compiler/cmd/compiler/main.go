package main

import (
	"flag"
	"fmt"
	"os"
)

func main() {
	helpFlag := flag.Bool("help", false, "Display help message")
	flag.Parse()

	args := flag.Args()
	var command string
	var files []string

	if len(args) > 0 {
		command = args[0]
		files = args[1:]
	}

	if command == "" {
		if *helpFlag {
			displayGeneralHelp()
			os.Exit(0)
		}
		fmt.Println("no command found")
		os.Exit(1)
	} else {
		if *helpFlag {
			displayGeneralHelp()
		}
		fmt.Printf("command: %s\n", command)
		fmt.Println(files)
	}
}

func displayGeneralHelp() {
	fmt.Print(`MiniJava compiler usage:
MiniJava [OPTIONS] COMMAND [files...]
COMMANDS:
	scan: scans the given files and prints the found tokens
OPTIONS:
	--help: Display this help message
`)
}