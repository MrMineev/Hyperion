use std::path::PathBuf;
use std::env;
use std::fs;

pub fn get_project_name() -> Option<String> {
    let current_dir = env::current_dir();

    match current_dir {
        Ok(path) => {
            let directory_name = path.file_name().unwrap_or_default();
            let directory_name_str = directory_name.to_string_lossy();

            return Some(directory_name_str.to_string());
        }
        Err(_err) => {
            return None;
        }
    }
}

pub fn create_folder(folder_name: String) -> bool {
    match fs::create_dir(folder_name) {
        Ok(_) => true,
        Err(_) => false,
    }
}

pub fn init_hypl() {
    if let Some(project_name) = get_project_name() {
        if create_folder("__modules__".to_string()) {
            println!("Success!");
        } else {
            eprintln!("An error accured while creating the folder {}", project_name);
        }
    }
}
