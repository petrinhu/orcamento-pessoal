function get_files
{
    echo x-skg.xml
}

function po_for_file
{
    case "$1" in
       x-skg.xml)
           echo skrooge_xml_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       x-skg.xml)
           echo comment
       ;;
    esac
}
