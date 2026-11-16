    public static String tabletIndexedArtFiles() {
        MainActivity activity=tabletPermissionActivity;
        if(activity==null || !tabletHasAllFilesAccess())return "[]";
        org.json.JSONArray found=new org.json.JSONArray();
        String[] suffixes={"kra","krz","ora","psd","png","jpg","jpeg","webp",
            "tif","tiff","bmp","gif","svg","exr","hdr","xcf","qoi",
            "jxl","heic","heif","avif"};
        StringBuilder selection=new StringBuilder();
        String[] args=new String[suffixes.length];
        for(int i=0;i<suffixes.length;i++) {
            if(i>0)selection.append(" OR ");
            selection.append(android.provider.MediaStore.Files.FileColumns.DISPLAY_NAME)
                     .append(" LIKE ?");
            args[i]="%."+suffixes[i]+(i<2?"%":"");
        }
        android.net.Uri base=android.provider.MediaStore.Files.getContentUri("external");
        String[] columns={
            android.provider.MediaStore.Files.FileColumns._ID,
            android.provider.MediaStore.Files.FileColumns.DISPLAY_NAME,
            android.provider.MediaStore.Files.FileColumns.DATA
        };
        try(android.database.Cursor cursor=activity.getContentResolver().query(
            base,columns,selection.toString(),args,
            android.provider.MediaStore.Files.FileColumns.DATE_MODIFIED+" DESC")) {
            if(cursor!=null)while(cursor.moveToNext() && found.length()<3500) {
                long key=cursor.getLong(0);
                String name=cursor.getString(1);
                String path=cursor.getString(2);
                org.json.JSONObject item=new org.json.JSONObject();
                item.put("uri",android.content.ContentUris.withAppendedId(
                    base,key).toString());
                item.put("name",name==null?"Artwork":name);
                item.put("path",path==null?"":path);
                found.put(item);
            }
        }catch(Exception ignored) {
            // Some Android versions/providers do not index arbitrary projects.
            // The Qt scanner and existing SAF history remain available.
        }
        return found.toString();
    }

