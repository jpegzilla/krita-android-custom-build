    public static String tabletProjectRoots() {
        org.json.JSONArray roots = new org.json.JSONArray();
        roots.put(Environment.getExternalStorageDirectory().getAbsolutePath());
        MainActivity activity = tabletPermissionActivity;
        if (activity != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            android.os.storage.StorageManager storage =
                activity.getSystemService(android.os.storage.StorageManager.class);
            if (storage != null) for (android.os.storage.StorageVolume volume : storage.getStorageVolumes()) {
                java.io.File directory = volume.getDirectory();
                if (directory != null && !volume.isPrimary()) roots.put(directory.getAbsolutePath());
            }
        }
        return roots.toString();
    }

    public static void tabletRememberProjectAccess(String location) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return;
        Uri uri = Uri.parse(location);
        if (!android.provider.DocumentsContract.isDocumentUri(activity, uri)) return;
        android.content.ContentResolver resolver = activity.getContentResolver();
        try {
            resolver.takePersistableUriPermission(uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        } catch (SecurityException readOnly) {
            try {
                resolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } catch (SecurityException temporary) {
                // Some providers offer only temporary access.
            }
        }
    }

    public static String tabletProjectName(String location) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return "";
        Uri uri = Uri.parse(location);
        String column = android.provider.OpenableColumns.DISPLAY_NAME;
        try (android.database.Cursor row = activity.getContentResolver().query(
                uri, new String[]{column}, null, null, null)) {
            if (row != null && row.moveToFirst() && !row.isNull(0)) return row.getString(0);
        } catch (RuntimeException ignored) {
            // A provider can disappear or revoke access.
        }
        return "";
    }

    public static long tabletProjectModified(String location) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return 0;
        Uri uri = Uri.parse(location);
        String column = DocumentsContract.Document.COLUMN_LAST_MODIFIED;
        try (android.database.Cursor row = activity.getContentResolver().query(
                uri, new String[]{column}, null, null, null)) {
            return row != null && row.moveToFirst() ? row.getLong(0) : 0;
        } catch (RuntimeException ignored) {
            return 0;
        }
    }

    public static boolean tabletDeleteProject(String location) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return false;
        Uri uri = Uri.parse(location);
        try {
            return android.provider.DocumentsContract.isDocumentUri(activity, uri) &&
                android.provider.DocumentsContract.deleteDocument(activity.getContentResolver(), uri);
        } catch (java.io.FileNotFoundException | RuntimeException ignored) {
            return false;
        }
    }

    public static boolean tabletRememberProjectFolder(Intent result) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null || result == null || result.getData() == null) return false;
        Uri tree = result.getData();
        if (!DocumentsContract.isTreeUri(tree)) return false;
        int flags = result.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                        Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if ((flags & Intent.FLAG_GRANT_READ_URI_PERMISSION) == 0) return false;
        try {
            activity.getContentResolver().takePersistableUriPermission(tree, flags);
            return true;
        } catch (SecurityException denied) {
            return false;
        }
    }

    private static boolean tabletIsFolderGrant(Uri uri) {
        return DocumentsContract.isTreeUri(uri) && uri.getPathSegments().size() == 2;
    }

    public static String tabletProjectFolders() {
        org.json.JSONArray folders = new org.json.JSONArray();
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return folders.toString();
        for (android.content.UriPermission grant : activity.getContentResolver().getPersistedUriPermissions()) {
            Uri uri = grant.getUri();
            if (!grant.isReadPermission() || !tabletIsFolderGrant(uri)) continue;
            try {
                org.json.JSONObject folder = new org.json.JSONObject();
                folder.put("uri", uri.toString());
                folder.put("name", DocumentsContract.getTreeDocumentId(uri) + "  (" + uri.getAuthority() + ")");
                folders.put(folder);
            } catch (Exception ignored) {
                // Stale grant.
            }
        }
        return folders.toString();
    }

    public static boolean tabletForgetProjectFolder(String location) {
        MainActivity activity = tabletPermissionActivity;
        if (activity == null || location == null) return false;
        Uri tree = Uri.parse(location);
        if (!tabletIsFolderGrant(tree)) return false;
        android.content.ContentResolver resolver = activity.getContentResolver();
        for (android.content.UriPermission grant : resolver.getPersistedUriPermissions()) {
            if (!tree.equals(grant.getUri())) continue;
            int flags = 0;
            if (grant.isReadPermission()) flags |= Intent.FLAG_GRANT_READ_URI_PERMISSION;
            if (grant.isWritePermission()) flags |= Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
            try {
                resolver.releasePersistableUriPermission(tree, flags);
                return true;
            } catch (SecurityException denied) {
                return false;
            }
        }
        return false;
    }

    private static final String[] tabletProjectColumns = {
        DocumentsContract.Document.COLUMN_DOCUMENT_ID,
        DocumentsContract.Document.COLUMN_DISPLAY_NAME,
        DocumentsContract.Document.COLUMN_MIME_TYPE,
        DocumentsContract.Document.COLUMN_LAST_MODIFIED
    };

    private static void tabletAppendProject(org.json.JSONArray projects,
            java.util.Set<String> seen, Uri uri, android.database.Cursor row,
            String folder) throws org.json.JSONException {
        String name = row.getString(1);
        if (name == null) return;
        String lower = name.toLowerCase(java.util.Locale.ROOT);
        if (!lower.matches(".*[.]kr[az]( \\(\\d+\\))?") &&
            !"application/x-krita".equals(row.getString(2))) return;
        if (!seen.add(uri.getAuthority() + "/" + row.getString(0))) return;
        org.json.JSONObject item = new org.json.JSONObject();
        item.put("uri", uri.toString());
        item.put("name", name);
        item.put("folder", folder);
        item.put("modified", row.getLong(3));
        if ("com.android.externalstorage.documents".equals(uri.getAuthority())) {
            String id = row.getString(0);
            int colon = id.indexOf(':');
            if (colon > 0) {
                String volume = id.substring(0, colon);
                java.io.File root = "primary".equals(volume) ?
                    Environment.getExternalStorageDirectory() : new java.io.File("/storage", volume);
                item.put("localPath", new java.io.File(root, id.substring(colon + 1)).getPath());
            }
        }
        projects.put(item);
    }

    private static void tabletScanProjectTree(android.content.ContentResolver resolver,
            Uri tree, org.json.JSONArray projects, java.util.Set<String> seen) {
        java.util.ArrayDeque<android.util.Pair<String, String>> folders = new java.util.ArrayDeque<>();
        java.util.HashSet<String> visited = new java.util.HashSet<>();
        String rootId = DocumentsContract.getTreeDocumentId(tree);
        String rootName = tree.getAuthority();
        Uri root = DocumentsContract.buildDocumentUriUsingTree(tree, rootId);
        try (android.database.Cursor row = resolver.query(root, tabletProjectColumns, null, null, null)) {
            if (row != null && row.moveToFirst() && row.getString(1) != null) rootName = row.getString(1);
        } catch (Exception ignored) {}
        folders.add(new android.util.Pair<>(rootId, rootName));
        while (!folders.isEmpty()) {
            android.util.Pair<String, String> folder = folders.removeFirst();
            if (!visited.add(folder.first)) continue;
            Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, folder.first);
            try (android.database.Cursor rows = resolver.query(children, tabletProjectColumns, null, null, null)) {
                if (rows == null) continue;
                while (rows.moveToNext()) {
                    String id = rows.getString(0);
                    if (id == null) continue;
                    if (DocumentsContract.Document.MIME_TYPE_DIR.equals(rows.getString(2))) {
                        String name = rows.getString(1);
                        if (!".krita-gallery-trash".equals(name))
                            folders.add(new android.util.Pair<>(id, folder.second + "/" + name));
                    } else {
                        tabletAppendProject(projects, seen,
                            DocumentsContract.buildDocumentUriUsingTree(tree, id), rows, folder.second);
                    }
                }
            } catch (Exception ignored) {
                // An unavailable child folder does not invalidate the whole tree.
            }
        }
    }

    public static String tabletGrantedProjects() {
        org.json.JSONArray projects = new org.json.JSONArray();
        MainActivity activity = tabletPermissionActivity;
        if (activity == null) return projects.toString();
        android.content.ContentResolver resolver = activity.getContentResolver();
        java.util.Set<String> seen = new java.util.HashSet<>();
        for (android.content.UriPermission permission : resolver.getPersistedUriPermissions()) {
            if (!permission.isReadPermission()) continue;
            Uri uri = permission.getUri();
            try {
                if (tabletIsFolderGrant(uri)) {
                    tabletScanProjectTree(resolver, uri, projects, seen);
                } else if (DocumentsContract.isDocumentUri(activity, uri)) {
                    try (android.database.Cursor row = resolver.query(uri, tabletProjectColumns, null, null, null)) {
                        if (row != null && row.moveToFirst())
                            tabletAppendProject(projects, seen, uri, row, uri.getAuthority());
                    }
                }
            } catch (Exception ignored) {
                // Grants may be stale or the provider may be offline.
            }
        }
        return projects.toString();
    }

    public static byte[] tabletKraPreview(String location) {
        MainActivity activity=tabletPermissionActivity;
        if(activity==null || location==null ||
           !location.startsWith("content://"))return null;
        try(java.io.InputStream input=activity.getContentResolver()
                .openInputStream(Uri.parse(location))) {
            if(input==null)return null;
            try(java.util.zip.ZipInputStream zip=new java.util.zip.ZipInputStream(
                    new java.io.BufferedInputStream(input,32768))) {
                for(int entries=0;entries<96;++entries) {
                    java.util.zip.ZipEntry entry=zip.getNextEntry();
                    if(entry==null)return null;
                    String name=entry.getName();
                    if("preview.png".equals(name) ||
                       "Thumbnails/thumbnail.png".equals(name)) {
                        java.io.ByteArrayOutputStream preview=
                            new java.io.ByteArrayOutputStream(32768);
                        byte[] buffer=new byte[16384];
                        int length;
                        while((length=zip.read(buffer))!=-1) {
                            if(preview.size()+length>8*1024*1024)return null;
                            preview.write(buffer,0,length);
                        }
                        return preview.toByteArray();
                    }
                    // Do not decompress huge merged-image entries just to
                    // reach a later thumbnail. Preview is normally early.
                    if(entry.getSize()>24L*1024*1024)return null;
                    zip.closeEntry();
                }
            }
        }catch(Exception ignored) {
            // URI access can be revoked or providers can expose bad ZIPs.
        }
        return null;
    }

    private static final String TAG