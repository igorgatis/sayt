# SAYT

TODO: Explain what SAYT is.

## Installation

There are multiple ways to install SAYT:

### Option A: Direct download (APE binary)

[sayt.com](https://github.com/igorgatis/sayt/releases/latest/download/sayt.com)
is an [Actually Portable Executable (APE)](https://justine.lol/ape.html) binary.
Download it and place it in the root of your repository. It works on Windows,
Linux, and macOS without modification.

### Option B: Wrapper scripts

If you'd rather not have a `sayt.com` binary checked in, use wrapper scripts:

- [saytw](https://github.com/igorgatis/sayt/releases/latest/download/saytw) for Linux/macOS
- [saytw.ps1](https://github.com/igorgatis/sayt/releases/latest/download/saytw.ps1) for Windows

**Linux/macOS:**

```sh
./saytw [args...]
```

**Windows:**

```powershell
.\saytw.ps1 [args...]
```

### Option C: Install via mise

If you use [mise](https://mise.jdx.dev/), you can install SAYT as a tool:

```sh
mise use github:igorgatis/sayt
```
