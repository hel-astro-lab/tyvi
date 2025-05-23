# Pre-commit hooks

Git hook scripts can be used to detect simple issues before commits.
Pre-commit hooks are run before every commit and they will prevent
the commit if the hooks find problems or if they make modification to the files
(e.g. clang-format).

`pre-commit` is a multi-language package manager for pre-commit hooks [^pre].
Tyvi uses `pre-commit` to install hooks defined in `.pre-commit-config.yaml`.

`pre-commit` can be installed using pip:

```shell
> pip install pre-commit
```

`pre-commit` git hook scripts can be installed with (in tyvi toplevel directory):

```shell
> pre-commit install
```

Now the installed hooks will run before each commit to the repository.
Note that the hooks are not pushed to remote repository.
One has to install them for each local repository.

Optionally functionality of the hooks can be tested with:

```shell
> pre-commit run --all-files
```


[^pre]: https://pre-commit.com/
