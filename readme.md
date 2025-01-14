# WB55 USB Device MTP example

The example is showing how to use MTP for read. 
In the device is create a storage structure to store content.
This is created in usbd_mtp_if.c file

by default there is created and file `text.txt`. With content `Hello`

files are created into `object` structure. If `object[x].Storage_id` is not **0** the object is used.
Each `object` can be file or folder. Currently only file is used. 
The file content is sotred in array `buffer`


At the beginning if the host call `USBD_MTP_Itf_GetIdx` with parameter 0xFFFFFFFF we will give him all IDs of the objects in ROOT. 
We will retrun it as array of `Storage_id`s. 

For example for two object the array will be like {object[0].Storage_id,object[1].Storage_id}

The argument `Param` is basically our `Storage_id` so each time we know which object to target. 

We can then give the host a names and sizes for each object. 

Or if neede we can create a new object if we have space and give to host a new ID. 

