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

### Example

```php
<?php
$augeas = new Augeas();

echo $augeas->get("/files/etc/hosts/1/ipaddr")."\n";
echo $augeas->get("/files/etc/hosts/1/canonical")."\n";
echo $augeas->get("/files/etc/hosts/1/alias")."\n";

$expectedArray = array(
            "/files/etc/hosts/1/ipaddr"    => "127.0.0.1",
            "/files/etc/hosts/1/canonical" => "localhost",
            "/files/etc/hosts/1/alias"     => "localhost.localdomain",
);
$matches = $augeas->match("/files/etc/hosts/1/*");
var_dump($matches);

echo $augeas->get("/files/etc/hosts/1/canonical")."\n";
$augeas->mv("/files/etc/hosts/1/canonical", "/files/etc/hosts/1/alias");
echo $this->augeas->get("/files/etc/hosts/1/alias")."\n";

?>
```
