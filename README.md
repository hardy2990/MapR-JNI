SUMMARY
-------
The ex folder contains source to perform operations using C-APIs against MapR-DB interactively with JNI.

INSTRUCTIONS
------------
In order to compile and run the example binary, follow these instructions:  

* Ensure $JAVA_HOME is exported and set correctly. Ex: /usr/lib/jvm/java-1.7.0-*/ OR  
   Ensure $JRE_LIB points to the directory containing the jvm library.
* Ensure $MAPR_HOME is exported and set correctly. Default: /opt/mapr
* Write you java code which will contain definitions of native functions to be used to call from the Java program (In this case JNIFoo.java)
* Compile the java code using javac command - javac JNIFoo
* Once compiled execute javah commad with your java class file name - javah JNIFoo. 
* Above step produces a header file which needs to be included in you C program - fooMapr.c
* Execute export LD_LIBRARY_PATH=/opt/mapr/lib:$JAVA_HOME/jre/lib/amd64/server
* Compile using g++ -shared -fpic -o libfoo.so -I $JAVA_HOME/include -I $JAVA_HOME/include/linux -I /opt/mapr/include -L /opt/mapr/lib -L /usr/lib/jvm/java-1.7.0-openjdk-1.7.0.99.x86_64/jre/lib/amd64/server -lMapRClient -ljvm fooMapR.c
* Above step produces a .so (shared object) file which is library that will be linked during the runtime in your java program
