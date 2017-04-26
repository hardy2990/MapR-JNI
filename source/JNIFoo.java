
public class JNIFoo {    
    public native String nativeFoo(String conn);    

    static {
        System.load("/c-api-sample-applications/interactive/ex/foo.so");
    }        

    public void print () {
    String str = nativeFoo("127.0.0.1:7222");
    System.out.println(str);
    }
    
    public static void main(String[] args) {
    (new JNIFoo()).print();
    return;
    }
}


