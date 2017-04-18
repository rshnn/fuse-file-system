# fuse-file-system


___ 

Process for running this shit: 

```
ssh into csuser

# clean the current contents of assignemnt.  fresh stub code
# dunnp why we have to do this all the time.  TODO 
# (on vm)
rm -rf assignment3
sh unzip.sh

# copy src into csuser (on home terminal) 
sh copy_src_over.sh

# configure and build (-f flag for foreground.  Shows fprintfs)
cd assignments3
./configure
make
touch testfile 
cd example
../src/sfs (-f) ../testfile mountdir

# profit
cat ../test
cat sfs.log
```