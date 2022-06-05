### (upcoming)

* Python 2 is no longer supported (Python 3.7 is now minimum).
* Patch number is now included in version strings (i.e. "0.3.12" instead of "0.3").

### 0.3.11

* Adds `nanojanskyToAbMag`, `nanojanksyToAbMagSigma`, `abMagToNanojansky` and `abMagToNanojanskySigma`
  photometry UDFs.
* Adds MySQL 8.0 compatibility (recent releases had only been being tested against MariaDB 10.x.)
* Adds CI for MariaDB, MySQL, and documentation builds via github actions.
* Latest documentation is now automatically published on github pages ([https://smonkewitz.github.io/scisql](https://smonkewitz.github.io/scisql)).

### 0.3.10

* Update use of deprecated method `xml.etree.ElementTree.Element.getiterator` in the docs.py Python
  documentation generation script.

### 0.3.9

* Upgrade waf to 2.0.15, which supports using Python 3.7 for builds.

