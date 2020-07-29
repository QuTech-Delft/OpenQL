Release procedure
=================

Most of the release process is managed by GitHub Actions. If you follow the
procedure below, it will automatically test and build all wheels and conda
packages for all platform, and publish them to PyPI and conda assuming the
secrets are configured correctly.

 - Make a branch with a name starting with "release" based upon the commit that
   is to be released.

 - Change the version in `src/version.h` (everything else that needs a version
   should derive from this) and change any other files where applicable
   (changelog, etc), then commit and make a PR for it.

 - CI will now run not only the test workflow, but also the assets workflow.
   The assets workflow builds the wheels and conda packages, but publishes them
   to the GitHub Actions build artifacts only. You can then test these yourself
   if you have reason to believe that something might be wrong with them that
   CI might not catch. To find them, go to Actions -> Assets workflow ->
   click the run for your branch -> Artifacts.

 - If the test or assets workflows fail, fix it before merging the PR (of
   course).

 - Once CI is green, merge the PR and delete the branch. Instead, push a
   tag with the appropriate version (customarily `x.y.z` without a `v` in
   front).

 - CI will automatically make a complete GitHub release for your tag. It will
   then run the assets workflow again, now with the new version string baked
   into the wheels and packages. When done, these wheels and packages are
   automatically added to the GitHub release, and if the secrets/API keys for
   PyPI and conda are correct, publish them there.
