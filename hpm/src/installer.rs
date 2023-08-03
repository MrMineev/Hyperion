use std::env;

use crate::download_package::*;

pub fn generate_link(web: String, project: String) -> String {
    format!("{}/{}/{}.zip", web, project, project)
}

pub fn install_package() {
    let hpm_link: String = "https://mrmineev.github.io/articles/hpm/packages".to_string();

    let args: Vec<String> = env::args().collect();
    let package_name: String = args[2].clone();
    let download_link: String = generate_link(hpm_link.clone(), package_name.clone());

    println!("[TODO] install: {}, link: {}", package_name, download_link);

    download_package(
        download_link.clone(),
        format!("__modules__/{}.zip", package_name.clone()),
        format!("__modules__"),
    );
}
