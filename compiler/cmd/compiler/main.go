package main

import (
	"flag"
	"fmt"
	"os"

	"github.com/MoustaphaSaad/taha2/compiler/internal/minijava"
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
	} else if command == "scan" {
		for _, file := range files {
			unit, err := minijava.LoadUnitFromFile(file)
			if err != nil {
				fmt.Printf("loading unit '%s' failed, %v", file, err)
				os.Exit(1)
			}

			err = unit.Scan()
			if err != nil {
				fmt.Printf("scanning unit '%s' failed, %v", file, err)
				os.Exit(1)
			}

			fmt.Println(unit.Tokens)
		}
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