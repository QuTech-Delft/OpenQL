Release procedure
=================

Most of the release process is managed by GitHub Actions. If you follow the
procedure below, it will automatically test and build all wheels and conda
packages for all platform, and publish them to PyPI and conda assuming the
secrets are configured correctly.

 - Make a branch with a name starting with "release" based upon the commit that
   is to be released. For example, `release-0.0.1`, but the suffix doesn't
   matter.

 - Change the version in `src/version.h` (this is the only place where the
   version is hardcoded) and change any other files where applicable
   (changelog, etc), then commit and make a PR for it.

 - CI will now run not only the test workflow, but also the assets workflow.
   The assets workflow builds the wheels and conda packages, but publishes them
   to the GitHub Actions build artifacts only. You can then test these yourself
   if you have reason to believe that something might be wrong with them that
   CI might not catch. To find them, go to Actions -> Assets workflow ->
   click the run for your branch -> Artifacts.

 - If the test or assets workflows fail, fix it before merging the PR (of
   course).

 - Once CI is green, merge the PR into `develop` if there are any changes.
   If no changes were needed, just delete the branch to clean up.

 - If needed, also merge to `master`.

 - Draft a new release through the GitHub interface. Set the "tag version"
   to the same version you put in `src/version.h`, the title to
   "Release `version`: `name`", and write a short changelog in the body.
   For releases that don't go to master, check the "prerelease" box, so it
   won't show up as the latest release.

 - CI will run the assets workflow again, now with the new version string baked
   into the wheels and packages. When done, these wheels and packages are
   automatically added to the GitHub release, and if the secrets/API keys for
   PyPI and conda are correct, CI will publish them there.
