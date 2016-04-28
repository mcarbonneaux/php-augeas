### new addition in this realese

- add dump_to_xml to retreave augeas tree in xml string from [aug_to_xml](http://augeas.net/docs/references/c_api/files/augeas-h.html#aug_to_xml)

### INSTALL

Install instructions (you’ll need php5 dev package):

```
$ pecl install augeas
```

or from the git repo:

```
$ git clone git://github.com/ppadron/php-augeas.git
$ cd php-augeas
$ phpize
$ ./configure
$ make
$ make install
```

Create a file called augeas.ini in your PHP conf.d directory containing:

```
extension=augeas.so
```

### API REFERENCE

```
void    Augeas::__construct([string $root[, string $loadpath[, int $flags]]])
string  Augeas::get(string $path)
string  Augeas::dump_to_xml(string $path)
array   Augeas::match(string $path);
boolean Augeas::set(string $path, string $value);
boolean Augeas::rm($augeas, string $path);
boolean Augeas::insert(string $path, string $label, int $order);
boolean Augeas::mv(string $source, string $destination);
boolean Augeas::save();
```

Constants used as $flags in Augeas::__construct()

```
Augeas::AUGEAS_NONE = 0
Augeas::AUGEAS_SAVE_BACKUP = 1
Augeas::AUGEAS_SAVE_NEWFILE = 2
Augeas::AUGEAS_TYPE_CHECK = 4
Augeas::AUGEAS_NO_STDINC = 8
```

Constants used as $order in Augeas::insert()

```
Augeas::AUGEAS_INSERT_BEFORE = 0
Augeas::AUGEAS_INSERT_AFTER = 1
```

### EXAMPLES

```php
<?php
$augeas = new Augeas();

// gets
echo $augeas->get("/files/etc/hosts/1/ipaddr")."\n";
echo $augeas->get("/files/etc/hosts/1/canonical")."\n";
echo $augeas->get("/files/etc/hosts/1/alias")."\n";

// dump_to_xml
echo $augeas->dump_to_xml("/files/etc/hosts/1/*")."\n";

// match
$matches = $augeas->match("/files/etc/hosts/1/*");
var_dump($matches);

// mv 
echo $augeas->get("/files/etc/hosts/1/canonical")."\n";
$augeas->mv("/files/etc/hosts/1/canonical", "/files/etc/hosts/1/alias");
echo $this->augeas->get("/files/etc/hosts/1/alias")."\n";

// rm and save
$augeas->rm('/files/etc/hosts/1/ipaddr');
$augeas->save();

// inserting before the first comment
$beforeFirstComment = "comment inserted before the first comment";
$firstComment       = "first comment";
$augeas->insert("/files/etc/hosts/#comment", "#comment", Augeas::AUGEAS_INSERT_BEFORE);
$augeas->set("/files/etc/hosts/#comment[1]", $beforeFirstComment);
echo $augeas->get("/files/etc/hosts/#comment[1]")."\n";
echo $augeas->get("/files/etc/hosts/#comment[2]")."\n";

?>
```
