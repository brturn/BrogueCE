rm bin/*depth\ 1.broguerec
rm bin/*depth\ 2.broguerec
rm bin/*depth\ 3.broguerec
rm bin/*depth\ 4.broguerec

cd bin

# Spawn a child process:
(./brogue $*) & pid=$!

echo $pid

# in the background, sleep for 10 secs then kill that process
# (sleep 300 && kill -9 $pid) &
cd ..



# Spawn a child process:
# (./brogue) & pid=$!
# in the background, sleep for 10 secs then kill that process
# (sleep 10 && kill -9 $pid) &
