package org.dltcollab;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class Dcurl {
    private static final String libraryName = "dcurl";
    private static final String libraryPrefix = "lib";
    private static final String librarySuffix = ".so";
    private static final String libraryFileName = libraryPrefix + libraryName + librarySuffix;

    public void loadLibraryFromJar() {
        final File temp;
        try {
            temp = File.createTempFile(libraryPrefix + libraryName, librarySuffix);
            if (temp.exists() && !temp.delete()) {
                throw new RuntimeException("File: " + temp.getAbsolutePath() + " already exists and cannot be removed.");
            }
            if (!temp.createNewFile()) {
                throw new RuntimeException("File: " + temp.getAbsolutePath() + " could not be created.");
            }

            if (!temp.exists()) {
                throw new RuntimeException("File " + temp.getAbsolutePath() + " does not exist.");
            } else {
                temp.deleteOnExit();
            }

            // attempt to copy the library from the Jar file to the temp destination
            try (final InputStream is = getClass().getClassLoader().getResourceAsStream(libraryFileName)) {
                if (is == null) {
                    throw new RuntimeException(libraryFileName + " was not found inside JAR.");
                } else {
                    Files.copy(is, temp.toPath(), StandardCopyOption.REPLACE_EXISTING);
                }
            }

            System.load(temp.getAbsolutePath());
        } catch (IOException e) {
        }
    }
}
