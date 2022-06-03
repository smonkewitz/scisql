### 0.3.11

* Adds `nanojanskyToAbMag`, `nanojanksyToAbMagSigma`, `abMagToNanojansky` and `abMagToNanojanskySigma`
  photometry UDFs.

* Adds MySQL 8.0 compatibility (recent releases had only been tested against MariaDB 10.x).  Added MariaDB,
  MySQL, and documentation CI builds via github actions.

* Documentation now published in github pages ([https://smonkewitz.github.io/scisql](https://smonkewitz.github.io/scisql))

### 0.3.10

* Update use of deprecated method `xml.etree.ElementTree.Element.getiterator` in the docs.py Python
  documentation generation script.

### 0.3.9

* Upgrade waf to 2.0.15, which supports using Python 3.7 for builds.

