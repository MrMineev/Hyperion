use std::env;
use std::fs::File;
use std::io::{Read, Write};
use serde_json::{json, Value};

use crate::download_package::*;

fn generate_link(web: String, project: String) -> String {
    format!("{}/{}/{}.zip", web, project, project)
}

fn update(package: String) -> Result<(), Box<dyn std::error::Error>> {
    // Read the JSON file
    let file_path = "__info__.json";
    let mut file = File::open(file_path)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;

    // Parse the JSON content into a serde_json::Value object
    let mut data: Value = serde_json::from_str(&contents)?;

    // Ensure that the data is an object (JSON object)
    if let Some(obj) = data.as_object_mut() {
        // Append a new module to the 'modules' array
        if let Some(modules) = obj.get_mut("modules") {
            if let Some(modules_array) = modules.as_array_mut() {
                modules_array.push(json!(package));
            }
        }
    }

    // Serialize the updated data back to JSON
    let new_contents = serde_json::to_string_pretty(&data)?;

    // Write the updated JSON back to the file
    let mut file = File::create(file_path)?;
    file.write_all(new_contents.as_bytes())?;

    Ok(())
}

pub fn install_package(package: String) {
    let hpm_link: String = "https://mrmineev.github.io/articles/hpm/packages".to_string();

    let args: Vec<String> = env::args().collect();
    let download_link: String = generate_link(hpm_link.clone(), package.clone());

    download_package(
        download_link.clone(),
        format!("__modules__/{}.zip", package.clone()),
        format!("__modules__"),
    );

    update(package.clone());
}

