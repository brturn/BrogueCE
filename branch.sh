# Create a new branch from an existing branch and switch to it
# branch <old> <new>
git checkout $1
git pull origin $1
git checkout -b $2
git branch --set-upstream-to=origin/$2 $2
git push --set-upstream origin $2
echo Created new branch $2 from upstream branch $1.
