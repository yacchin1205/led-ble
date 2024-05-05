# nginxの公式イメージをベースとする
FROM nginx:alpine

# デフォルトのnginxの設定ファイルを削除
RUN rm /etc/nginx/conf.d/default.conf

# カスタム設定ファイルをコピー
COPY nginx.conf /etc/nginx/conf.d/

# ホストのファイル（index.htmlなど）をコンテナ内の適切なディレクトリにコピー
COPY ./www /usr/share/nginx/
