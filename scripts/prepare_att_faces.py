#!/usr/bin/env python3
"""Verify and split the official AT&T / ORL archive for the Qt PCA workbench.

Uses the Python standard library only. Download the official archive separately;
this script never fetches network resources or changes the source images.
"""

import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import zipfile


ARCHIVE_SHA256 = "509d763bd2276aa054d260d7f1000e8be212738134f2f83d8ff13bc2adf1c8ec"
SOURCE_PAGE = "https://www.cl.cam.ac.uk/research/dtg/attarchive/facedatabase.html"
DOWNLOAD_URL = "https://www.cl.cam.ac.uk/Research/DTG/attarchive/pub/data/att_faces.zip"


def write_identical_or_new(path, data):
    """Rerunning is safe; edited files are never silently overwritten."""
    if path.exists():
        if path.read_bytes() != data:
            raise ValueError("Refusing to overwrite a different file: {}".format(path))
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def prepare(archive_path, destination):
    archive_data = archive_path.read_bytes()
    archive_hash = hashlib.sha256(archive_data).hexdigest()
    if archive_hash != ARCHIVE_SHA256:
        raise ValueError("Archive SHA-256 does not match the verified official download")

    rows = []
    payloads = []
    seen_hashes = set()
    header = re.compile(rb"P5\s+92\s+112\s+255\s")
    with zipfile.ZipFile(io.BytesIO(archive_data)) as archive:
        invalid_member = archive.testzip()
        if invalid_member:
            raise ValueError("Archive CRC failed: {}".format(invalid_member))
        expected = {"s{}/{}.pgm".format(person, image)
                    for person in range(1, 41) for image in range(1, 11)}
        actual = {name for name in archive.namelist() if name.lower().endswith(".pgm")}
        if actual != expected:
            raise ValueError("Archive must contain exactly 40 people x 10 expected PGM images")
        for person in range(1, 41):
            label = "s{:02d}".format(person)
            for image in range(1, 11):
                member = "s{}/{}.pgm".format(person, image)
                data = archive.read(member)
                match = header.match(data)
                if match is None or len(data) - match.end() != 92 * 112:
                    raise ValueError("Invalid 92x112, 8-bit PGM: {}".format(member))
                digest = hashlib.sha256(data).hexdigest()
                if digest in seen_hashes:
                    raise ValueError("Duplicate source image: {}".format(member))
                seen_hashes.add(digest)
                split = "train" if image <= 7 else "validation" if image == 8 else "test"
                relative = "{}/{}/{:02d}.pgm".format(split, label, image)
                payloads.append((destination / relative, data))
                rows.append({"split": split, "person": label, "source_member": member,
                             "relative_path": relative, "width": 92, "height": 112,
                             "sha256": digest})
        payloads.append((destination / "source" / "ORIGINAL_README.txt", archive.read("README")))

    # Build all metadata before writing anything. Archive names are never extracted
    # directly; all output paths above come from fixed numeric identity/image ranges.
    manifest = io.StringIO(newline="")
    writer = csv.DictWriter(manifest, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
    metadata = {
        "dataset": "AT&T Database of Faces (formerly ORL Database of Faces)",
        "source_page": SOURCE_PAGE,
        "download_url": DOWNLOAD_URL,
        "archive_sha256": archive_hash,
        "archive_bytes": len(archive_data),
        "people": 40,
        "images": 400,
        "image_format": "PGM P5, 92x112, 8-bit grayscale",
        "label_mapping": "s01..s40 map to original s1..s40; anonymous dataset IDs, not names",
        "splits": {
            "train": {"people": 40, "images_per_person": 7, "images": 280, "original_image_numbers": [1, 2, 3, 4, 5, 6, 7]},
            "validation": {"people": 40, "images_per_person": 1, "images": 40, "original_image_numbers": [8]},
            "test": {"people": 40, "images_per_person": 2, "images": 80, "original_image_numbers": [9, 10]},
        },
        "protocol": "Closed-set identity recognition: all 40 people are enrolled; no unknown identities in these splits.",
        "credit": ["AT&T Laboratories Cambridge", "Olivetti Research Laboratory"],
        "reference": "F. Samaria and A. Harter, Parameterisation of a stochastic model for human face identification, 2nd IEEE Workshop on Applications of Computer Vision, December 1994.",
    }
    payloads.append((destination / "manifest.csv", manifest.getvalue().encode("utf-8-sig")))
    payloads.append((destination / "dataset.json", (json.dumps(metadata, ensure_ascii=False, indent=2) + "\n").encode("utf-8")))
    for path, data in payloads:
        if path.exists() and path.read_bytes() != data:
            raise ValueError("Refusing to overwrite a different file: {}".format(path))
    for path, data in payloads:
        write_identical_or_new(path, data)
    print(json.dumps({"destination": str(destination.resolve()), "people": 40,
                      "train_images": 280, "validation_images": 40, "test_images": 80,
                      "unique_source_images": len(seen_hashes), "archive_sha256": archive_hash},
                     ensure_ascii=False, indent=2))


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, default=project / "data/att_faces/source/att_faces.zip")
    parser.add_argument("--output", type=Path, default=project / "data/att_faces")
    args = parser.parse_args()
    try:
        prepare(args.archive.resolve(), args.output.resolve())
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, "ERROR: {}\n".format(error))


if __name__ == "__main__":
    main()
