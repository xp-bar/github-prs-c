Github PRs
c ncurses terminal program to display your current PRs in your terminal.

To install:
create a github api token here: https://github.com/settings/tokens/new
then run the install command (clone repo, cd into dir, install.sh (makes a symlink to your `/usr/local/bin` dir so you can run it anywhere))
`ogdir=$PWD; git clone https://github.com/xp-bar/github-prs-c ~/github-prs-c; cd ~/github-prs-c; ./install.sh; cd $ogdir`
to run, either set env vars (run `prs` to see instructions), or pass as params:
`prs -c -u xp-bar -r owner/repo`
