STARDUST=/mnt/c/Users/zache/OneDrive/Desktop/Ninja/Github/Stardust/files

make -j${nproc}
echo 'Build finsihed. Copying REL to Stardust'
cp mkb2.rel_sample.rel "$STARDUST"
echo 'Done.'