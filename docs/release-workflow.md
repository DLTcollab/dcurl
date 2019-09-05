# Release Workflow

The motivation of release workfow is to update the GitHub release page
when there is a new tag pushed to the GitHub.

The release workflow uses `auto-changelog` utility to generate the
changelog, then use `release-it` utility to publish the changelog
on the release page.

The `release-it` will also build the `remote-worker`, and upload the
packed the `remote-worker` to release page.

- `.release-it`: `release-it` configuration file
- `.auto-changelog`: `auto-changelog` configuration file
- `changelog.hbs`: changelog template file

## Reference

- [release-it/release-it](https://github.com/release-it/release-it)
- [CookPete/auto-changelog](https://github.com/CookPete/auto-changelog)
