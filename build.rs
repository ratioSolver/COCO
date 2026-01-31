fn main() {
    println!("cargo:rerun-if-env-changed=CLIPS_LIB_DIR");
    match std::env::var("CLIPS_LIB_DIR") {
        Ok(lib_dir) => {
            println!("cargo:rustc-link-search=native={}", lib_dir);
        }
        Err(_) => {
            let common_paths = [
                "/usr/lib",
                "/usr/lib64",
                "/usr/local/lib",
                "/usr/lib/x86_64-linux-gnu",
            ];
            for path in common_paths {
                if std::path::Path::new(path).exists() {
                    println!("cargo:rustc-link-search=native={}", path);
                }
            }
        }
    }
}
