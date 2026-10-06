# Contributing to DCTelnet

Thanks for your interest in contributing to DCTelnet. For significant changes, open an issue first to discuss the proposed approach. Keep pull requests focused and avoid unrelated changes.

## Build and verify

The official DCTelnet build method, also used by CI, is: `make ci`

It builds the DCTelnet and ibmcon.device binaries, runs the tests, and packages an LHA archive containing everything needed to install DCTelnet on an Amiga.

- The build uses the VBCC Docker builder image: [walkero/docker4amigavbcc](https://hub.docker.com/r/walkero/docker4amigavbcc)
- Git, GNU Make and Docker are required; WSL is also required on Windows.
- You do not need to install a C compiler locally.

From the repository root, initialize the pinned submodule revisions and build:

```sh
git submodule update --init --recursive
cd src
make ci
```

This checks out the submodule revisions selected by the repository. The build outputs are written to `build/`.

Before considering code changes complete, run `make ci` and report the result. Documentation-only changes do not require a build. If the build cannot be run for another reason, explain why.

GitHub Actions runs this build for pull requests.

## Open a pull request

- Base your work on the current development branch, `v2.0-dev`, and target pull requests to
  `v2.0-dev`, unless maintainers request otherwise.
- Use a clear title and explain the motivation and impact of the change.
- Link a related issue when applicable.
- Include the build or tests you ran and their results. If you could not run them, mention why.
- Update relevant documentation when behavior, build steps, or user-facing features change.
- Do not include generated binaries or other build output in the pull request.

## Architecture

The application is written in C, using the C99 features supported by VBCC, and split into modules under `src/`:

- `DCTelnet.c` coordinates application startup, the main interface.
- `connect.c` implements the connection-progress window. `guis.c` contains the address book, profile editor, function-key editor, and scrollback window.
- `prefs.c` and `prefs_file.c` manage preferences and their on-disk format. `requesters.c` provides native AmigaOS requester helpers, while `utils.c` contains shared utilities.
- `Xfer.c` integrates XPR file transfers. `Xem_wrapper.c` adapts the XEM library callbacks.
- The `petscii_*.c` modules handle PETSCII character conversion and dispatch.

The main VBCC/GCC build is defined in `src/Makefile`. Keep module interfaces in their corresponding headers and avoid coupling unrelated modules.

## Coding conventions

- For C source files, use ISO-8859-1 encoding, LF line endings, and four spaces per indentation level.
- Apply the following naming convention to new code and future changes. Existing code is not yet fully consistent, so this is not a request to rename unrelated legacy code. The convention is inspired by the Amiga NDK:
	- Use uppercase Amiga types such as `BYTE`, `BOOL`, and `ULONG`.
	- Use lower camel case for variables, including globals, and prefix Boolean names with `is`, `has`, `can`, or `should` where appropriate: `myGlobalVariable`, `isAppIconified`, `shouldIconify`.
	- Use PascalCase for function names and custom structure names: `HasNonZeroByte`, `MyCustomStructure`.
	- Use PascalCase for fields in custom structures, matching Intuition structures.

	```c
	BYTE myGlobalVariable;
	BOOL isAppIconified;

	BOOL HasNonZeroByte(const UBYTE *buffer, ULONG bufferLength)
	{
		ULONG byteIndex;

		if (buffer == NULL)
			return FALSE;

		for (byteIndex = 0; byteIndex < bufferLength; byteIndex++)
		{
			if (buffer[byteIndex] != 0)
				return TRUE;
		}

		return FALSE;
	}

	struct MyCustomStructure
	{
		BOOL IsEchoEnabled;
		APTR Context;
	};
	```

- Follow nearby formatting and error-handling patterns; keep changes focused.
- Use AmigaOS types and APIs consistently with nearby code. Check allocation and library-opening results, and respect caller-provided buffer sizes.
- Avoid new dependencies unless they are needed and compatible with the supported toolchains.

## Compatibility

DCTelnet targets classic Amiga systems with a Motorola 68000 or later and AmigaOS 2.00 (Kickstart 36) or later. The official build produces separate 68000 and 68020 release binaries.

Future changes should preserve this goal: **keep DCTelnet lightweight and responsive** on classic Amiga hardware, including an Amiga 500 with its 7 MHz 68000, without requiring excessive CPU power or memory.

Preserve 68000 compatibility and the AmigaOS 2.00 baseline. If a change needs a newer CPU instruction, operating-system feature, or library version, guard its use and provide a compatible fallback where practical. Verify optional library and API availability at runtime rather than assuming it is installed.
