# multi-langs support

domain_name="annotation-tool"

FILE_LIST="src/*.c"

xgettext -C  --keyword=_  \
    -o ${domain_name}.pot \
    --default-domain="annotation-tools" \
    --from-code=utf-8 \
    -c \
    --package-name=annotation-tools \
    --package-version=0.1.0 \
    --msgid-bugs-address=htc.chehw@gmail.com \
    ${FILE_LIST}

    
msginit --no-translator --input=${domain_name}.pot --locale=ja_JP.utf-8

mkdir -p lang/jp/LC_MESSSAGES
msgfmt --output-file=lang/ja/LC_MESSSAGES/${domain_name}.mo ${domain_name}.po
