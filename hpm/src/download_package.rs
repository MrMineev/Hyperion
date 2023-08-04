// Import the reqwest crate
use reqwest::blocking::Client;
use std::fs;
use std::fs::File;
use std::io::{Read, Write};
use zip::read::ZipArchive;

fn delete_file(file_path: &str) -> Result<(), Box<dyn std::error::Error>> {
    fs::remove_file(file_path)?;
    Ok(())
}

fn unzip_file(zip_path: &str, output_dir: &str) -> Result<(), Box<dyn std::error::Error>> {
    let file = File::open(zip_path)?;
    let mut archive = ZipArchive::new(file)?;

    for i in 0..archive.len() {
        let mut file = archive.by_index(i)?;
        let out_path = format!("{}/{}", output_dir, file.name());

        if (&*file.name()).ends_with('/') {
            std::fs::create_dir_all(&out_path)?;
        } else {
            if let Some(p) = out_path.rfind('/') {
                if p > 0 {
                    std::fs::create_dir_all(&out_path[..p])?;
                }
            }

            let mut outfile = File::create(&out_path)?;
            std::io::copy(&mut file, &mut outfile)?;
        }
    }

    Ok(())
}

pub fn download_package(url: String, name: String, output_dir: String) {
    let client = Client::new();
    match client.get(&url).send() {
        Ok(response) => {
            if response.status().is_success() {
                let body = response.bytes().expect("Failed to read response body");
                let mut file = File::create(&name).expect("Failed to create file");
                file.write_all(&body).expect("Failed to write to file");
            } else {
                println!("Request failed with status code: {}", response.status());
            }
        }
        Err(e) => {
            println!("Error occurred: {:?}", e);
        }
    }
    unzip_file(&name, &output_dir);

    delete_file(&name);
}

