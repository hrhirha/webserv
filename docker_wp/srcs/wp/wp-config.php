<?php
/**
 * The base configuration for WordPress
 *
 * The wp-config.php creation script uses this file during the
 * installation. You don't have to use the web site, you can
 * copy this file to "wp-config.php" and fill in the values.
 *
 * This file contains the following configurations:
 *
 * * MySQL settings
 * * Secret keys
 * * Database table prefix
 * * ABSPATH
 *
 * @link https://codex.wordpress.org/Editing_wp-config.php
 *
 * @package WordPress
 */

// ** MySQL settings - You can get this info from your web host ** //
/** The name of the database for WordPress */
define( 'DB_NAME', 'wp_db' );

/** MySQL database username */
define( 'DB_USER', 'admin' );

/** MySQL database password */
define( 'DB_PASSWORD', 'admin' );

/** MySQL hostname */
define( 'DB_HOST', 'localhost' );

/** Database Charset to use in creating database tables. */
define( 'DB_CHARSET', 'utf8' );

/** The Database Collate type. Don't change this if in doubt. */
define( 'DB_COLLATE', '' );

/**#@+
 * Authentication Unique Keys and Salts.
 *
 * Change these to different unique phrases!
 * You can generate these using the {@link https://api.wordpress.org/secret-key/1.1/salt/ WordPress.org secret-key service}
 * You can change these at any point in time to invalidate all existing cookies. This will force all users to have to log in again.
 *
 * @since 2.6.0
 */
define('AUTH_KEY',         '=*A>LfU,(w$;Z,s>EWRAU*APP99Y;W1OxcHw>D>om|lkxco9s:R|<![eK_C^?t6S');
define('SECURE_AUTH_KEY',  'n+6SCi:CW]BI$VI)E d(|7V?IU%+rWQv)k{[dv]6aR2%`+{PqM1dITR^V,Q4*jrC');
define('LOGGED_IN_KEY',    '/%F?`gXSZSB%$U44VkqM@&)&DUG4nuyPFc,*j]r_Ekst{tNa?Uv8l#C{8/r&[+Y]');
define('NONCE_KEY',        'z,1evLEiB=U+|3|/GMZ3na1m8F/z$e|`A/j :n-qoc5sihJ`mLP+EPc+_~kL=Lz[');
define('AUTH_SALT',        'e_dSm4CL!xW]|:`3}}&t~;r|Yk)YwQ6RTAw/M??4/_7h]nDu=/diR+,d+P5+78[&');
define('SECURE_AUTH_SALT', ' lkueBpzPf6OAe%{tY;N|ls^C1Xp8fJ|@~^D<zq@z@*tyVBqzCVm,mz49viS6So|');
define('LOGGED_IN_SALT',   '3}G>f}d^s!I|}<MrsoZ0f&typd4TUn$7?kJvWY+XSsVt6>^vI-1%b_Txz8T``=~o');
define('NONCE_SALT',       'H}1B*$.e25mPO5*n5.F=4o}1L9Fp>Py,Go%|Ipt_(07{X~x]=<uq@Rrkv8rPYn@~');
/**#@-*/

/**
 * WordPress Database Table prefix.
 *
 * You can have multiple installations in one database if you give each
 * a unique prefix. Only numbers, letters, and underscores please!
 */
$table_prefix = 'wp_';

/**
 * For developers: WordPress debugging mode.
 *
 * Change this to true to enable the display of notices during development.
 * It is strongly recommended that plugin and theme developers use WP_DEBUG
 * in their development environments.
 *
 * For information on other constants that can be used for debugging,
 * visit the Codex.
 *
 * @link https://codex.wordpress.org/Debugging_in_WordPress
 */
define( 'WP_DEBUG', false );

/* That's all, stop editing! Happy publishing. */

/** Absolute path to the WordPress directory. */
if ( ! defined( 'ABSPATH' ) ) {
	define( 'ABSPATH', dirname( __FILE__ ) . '/' );
}

/** Sets up WordPress vars and included files. */
require_once( ABSPATH . 'wp-settings.php' );
