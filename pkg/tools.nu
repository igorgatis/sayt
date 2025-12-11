export def --wrapped vrun [--trail="\n", cmd, ...args] {
  let quoted_args = $args | each { |arg|
    if ($arg | into string | str contains ' ') { $arg | to nuon } else { $arg } }
  print -n $"($cmd) ($quoted_args | str join ' ')($trail)"
  $in | ^$cmd ...$args
}

export def --wrapped _cue [...args] {
  let stub = $env.FILE_PWD | path join "cue.toml"
  vrun mise tool-stub $stub ...$args
}

export def --wrapped _uvx [...args] {
  let stub = $env.FILE_PWD | path join "uvx.toml"
  vrun mise tool-stub $stub ...$args
}

export def --wrapped _docker [...args] {
  let stub = $env.FILE_PWD | path join "docker.toml"
  vrun mise tool-stub $stub ...$args
}

export def --wrapped _docker_compose [...args] {
  let stub = $env.FILE_PWD | path join "docker.toml"
  vrun mise tool-stub $stub compose ...$args
}

export def --wrapped _nu [...args] {
  let stub = $env.FILE_PWD | path join "nu.toml"
  vrun mise tool-stub $stub ...$args
}
