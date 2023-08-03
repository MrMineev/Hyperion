use std::env;
use std::fs;
use std::collections::HashMap;
use std::fs::File;
use std::io::Write;
use std::any::Any;

use serde_json::{json, Value};

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
        if !create_folder("__modules__".to_string()) {
            eprintln!("An error accured while creating the folder {}", project_name);
        }
        let data = json!({
            "name": project_name.clone(),
            "modules": []
        });

        let json_string =
            serde_json::to_string_pretty(&data).expect("Failed to serialize JSON");
        let mut file = File::create("__info__.json").expect("Failed to create output JSON file");
        file.write_all(json_string.as_bytes()).expect("Failed to write output JSON file");
    }
}











