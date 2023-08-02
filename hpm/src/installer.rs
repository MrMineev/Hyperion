use std::env;

pub fn install_package() {
    let args: Vec<String> = env::args().collect();
    let package_name: String = args[2].clone();

    println!("[TODO] install {}", package_name);
}
