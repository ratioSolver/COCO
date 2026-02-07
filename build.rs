fn main() {
    println!("cargo:rustc-link-search=native=./clips");
    println!("cargo:rustc-link-lib=clips");
    let bindings = bindgen::Builder::default().header("wrapper.h").clang_arg("-Iclips").generate().expect("Unable to generate bindings");

    bindings.write_to_file("src/adapters/clips_bindings.rs").expect("Couldn't write bindings!");
}
