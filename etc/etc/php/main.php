<?php


function join_path(string... $args)
{
    $paths = [];
    foreach($args as $arg)
    {
        //$paths = array_merge($paths, (array)$arg);
        if(!empty($arg))
        {
            array_push($paths, $arg);
        }
    }
    foreach($paths as &$path)
    {
        $path = trim($path, '/');
    }
    if(substr($args[0], 0, 1) == '/')
    {
        $paths[0] = '/' . $paths[0];
    }
    return join('/', $paths);
}

$dir = $argv[1];

// Open a known directory, and proceed to read its contents
if (is_dir($dir))
{
    if ($dh = opendir($dir))
    {
        while(($item = readdir($dh)) !== false)
        {
            if(($item == ".") || ($item == ".."))
            {
                continue;
            }
            $fullp = join_path($dir, $item);
            // Possible values are fifo, char, dir, block, link, file, socket and unknown
            $typ = @filetype($fullp);
            $isfile = is_file($fullp);
            $isdir = is_dir($fullp);
            #echo "item $fullp: filetype=" . $typ . "\n";
            printf("item %s: filetype=%s\n", $fullp, $typ);
        }
        closedir($dh);
    }
}



