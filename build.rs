fn main() {
    println!("cargo:rerun-if-env-changed=CLIPS_LIB_DIR");
    if let Ok(lib_dir) = std::env::var("CLIPS_LIB_DIR") {
        println!("cargo:rustc-link-search=native={}", lib_dir);
    }
}
