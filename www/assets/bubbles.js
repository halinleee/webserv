(function () {
    var container = document.getElementById('oceanBg');
    if (!container) return;

    // 장식 이미지(배경 제거된 캐릭터 스티커)를 페이지 로드 시 한 번 배치
    // 마녀 패트릭은 하늘을 날아다니고, 나머지는 배경 그림의 언덕(땅) 라인 근처에 자리잡음
    var corners = [
        { file: '1.png', pos: 'sky-flyer' },
        { file: '3.png', pos: 'ground-left' },
        { file: '4.png', pos: 'ground-center' },
        { file: '2.png', pos: 'ground-right' }
    ];
    corners.forEach(function (deco) {
        if (deco.pos === 'sky-flyer') {
            // 이동/회전(wrapper)과 좌우반전(내부 img)의 transform이 서로 부딪히지 않도록 분리
            var wrap = document.createElement('div');
            wrap.className = 'corner-deco ' + deco.pos;

            var flyerImg = document.createElement('img');
            flyerImg.className = 'sticker';
            flyerImg.src = '/assets/images/' + deco.file;
            flyerImg.alt = '';
            wrap.appendChild(flyerImg);
            container.appendChild(wrap);

            // 숨겨진 이스터에그: 클릭하면 이동 (시각적 힌트 없음)
            wrap.addEventListener('click', function () {
                window.location.href = '/easter-egg.html';
            });

            var flipCount = 0;
            wrap.addEventListener('animationiteration', function () {
                flipCount++;
                // alternate: 홀수 번째 왕복(오른쪽->왼쪽 구간)일 때만 반전
                wrap.classList.toggle('facing-left', flipCount % 2 === 1);
            });
            return;
        }

        var img = document.createElement('img');
        img.className = 'corner-deco sticker ' + deco.pos;
        img.src = '/assets/images/' + deco.file;
        img.alt = '';
        container.appendChild(img);
    });

    function spawnBubble() {
        var bubble = document.createElement('div');
        bubble.className = 'bubble';

        var size = 6 + Math.random() * 26;
        var left = Math.random() * 100;
        var duration = 6 + Math.random() * 8;
        var drift = (Math.random() * 80 - 40) + 'px';

        bubble.style.width = size + 'px';
        bubble.style.height = size + 'px';
        bubble.style.left = left + 'vw';
        bubble.style.setProperty('--drift', drift);
        bubble.style.animationDuration = duration + 's';

        container.appendChild(bubble);
        setTimeout(function () {
            bubble.remove();
        }, duration * 1000);
    }

    for (var i = 0; i < 12; i++) {
        setTimeout(spawnBubble, i * 400);
    }

    setInterval(spawnBubble, 700);
})();
