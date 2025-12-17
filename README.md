# SAYT

TODO: Explain what SAYT is.

## Installation

There are multiple ways to install SAYT:

### Option A: Direct download (APE binary)

Download `sayt.com` directly and place it in your repository root or `~/.local/bin`:

```sh
curl -fsSL -o sayt.com https://github.com/igorgatis/sayt/releases/latest/download/sayt.com
chmod +x sayt.com
```

This is an [Actually Portable Executable (APE)](https://justine.lol/ape.html)
binary which works on Windows, Linux, and macOS without modification.

### Option B: Wrapper scripts

Download the wrapper scripts to your repository. These scripts automatically
download `sayt.com` to a cache directory on first run.

**For Linux/macOS (shell script):**

```sh
curl -fsSL -o saytw https://github.com/igorgatis/sayt/releases/latest/download/saytw
chmod +x saytw
./saytw [args...]
```

**For Windows (PowerShell):**

```powershell
Invoke-WebRequest -Uri https://github.com/igorgatis/sayt/releases/latest/download/saytw.ps1 -OutFile saytw.ps1
.\saytw.ps1 [args...]
```

The wrapper scripts:
1. Check for `sayt.com` in the current directory
2. Check for `sayt.com` in the cache directory
3. Check if `sayt.com` is available in PATH
4. If not found, download `sayt.com` to the cache directory
5. Execute `sayt.com` with the provided arguments

### Option C: Install via mise

If you use [mise](https://mise.jdx.dev/), you can install SAYT as a tool:

```sh
mise use github:igorgatis/sayt
```
