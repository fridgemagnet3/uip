#!/usr/bin/python3
import os
import subprocess

# reimplementaton of 'makefsdata' to allow for programmatic initialisation
# of the structures since CMOC doesn't allow for this to be generated
# at compile time

fo = open("httpd-fsdata.c","w")
fo.write("// File automatically generated from makefsdata.py\n\n")

# generate list of files which are served up
fs_folder = "./httpd-fs/"
fs_list = []
for fi in os.listdir(fs_folder):
    if os.path.isfile(os.path.join(fs_folder,fi)):
        fs_list.append(fi)

f_arrays = []
# walk the file list
for f in fs_list:
    print("Adding " + f)
    # generate 'C' array definition from the file
    cproc = subprocess.run(['/usr/bin/xxd','-i',os.path.join(fs_folder,f)],capture_output=True,text=True)
    lines = cproc.stdout.splitlines()
    # iterate thru the 'xxd' o/p
    for line in lines:
        if '};' in line:
            # null terminate the string
            fo.write(", 0x0 };\n\n")
            # then bail, discarding the rest of the o/p
            break
        else:
            fo.write(line+"\n")
            # write the hex representation of the filename at the start of the array
            if 'unsigned' in line:
                # prefix with a "/"
                fo.write("  " + hex(ord('/')) + ", ")
                for c in f:
                    fo.write(hex(ord(c)) + ", ")
                fo.write("0x0,\n")
                # add to list of array names
                c_decl = line.split()
                f_arrays.append(c_decl[2][:-2])

# declare the structures which will ultimately point to the arrays above
for f in f_arrays:
    ds_name = "file" + f
    fo.write("static struct httpd_fsdata_file " + ds_name + ";\n\n")

fo.write("#define HTTPD_FS_ROOT &" + ds_name + "\n\n")
fo.write("#define HTTPD_FS_NUMFILES " + str(len(f_arrays)) + "\n\n")

# define the init procedure
fo.write("static void init_httpd_fsdata(void)\n{\n")
idx = 0
prev = "NULL"
# now generate the code to initialise the structures since we can't do
# this at compile time
for f in f_arrays:
    # structure name
    ds_name = "file" + f
    out_ds_name = "  " + ds_name
    fo.write(out_ds_name + ".next = " + prev + ";\n")
    prev = "&" + ds_name
    # pointer to the filename
    fo.write(out_ds_name + ".name = (const char*)" + f + ";\n")
    # length of the filename at the start of each of the arrays
    # + leading "/" + null terminator
    fname_len = str(len(fs_list[idx])+2)
    # pointer to the file content
    fo.write(out_ds_name + ".data = (const char*)" + f + " + " + fname_len + ";\n")
    # length of the filename
    fo.write(out_ds_name + ".len = sizeof(" + f + ") - " + fname_len + ";\n\n")
    idx+=1
fo.write("}\n") ;
print("done")
