#current dir is linux source 

#apply 2.6.35 android update
if [ -f ./patch/linux-2.6.35_patch/apply.sh ]; then
    sh ./patch/linux-2.6.35_patch/apply.sh
fi

