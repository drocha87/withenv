# withenv

`withenv` is a simple utility program that reads environment variables from a file and injects them into a specified command. This is useful for setting environment variables from a file before running a command.

## Features

- Reads environment variables from a file.
- Injects the variables into the environment of the specified command.
- Supports passing arguments to the command.

## Installation

1. Clone the repository:

```sh
git clone https://github.com/drocha/withenv.git
```

2. Navigate to the project directory:

```sh
cd withenv
```

3. Compile the program:

```sh
make
```

## Usage

```sh
withenv env_file command [args...]
```

- `env_file`: Path to the environment file containing key=value pairs
- `command`: The command to be executed with the specified environment variables
- `args...`: Arguments to be passed to the command

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Credits

This project uses a subset of [nobuild](https://github.com/tsoding/nobuild) library from [Tsoding organization](https://github.com/tsoding). Special thanks to him for his valuable work and contribution.
