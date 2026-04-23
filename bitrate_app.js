document.addEventListener('DOMContentLoaded', () => {
    const tabs = document.querySelectorAll('.tab');
    const pauseBtn = document.getElementById('pauseBtn');
    const panelCards = document.querySelectorAll('.panel-card');
    
    let isPaused = false;

    // Tab Switching Logic
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            
            // In a real app, this would change the base image/video source
            console.log(`Switched to tab: ${tab.dataset.tab}`);
            
            // Visual feedback for switching
            const images = document.querySelectorAll('.video-container img');
            images.forEach(img => {
                img.style.opacity = '0';
                setTimeout(() => {
                    img.style.opacity = '1';
                }, 200);
            });
        });
    });

    // Pause Button Logic
    pauseBtn.addEventListener('click', () => {
        isPaused = !isPaused;
        pauseBtn.innerHTML = isPaused ? 
            '<span class="icon">▶</span> Resume' : 
            '<span class="icon">⏸</span> Pause';
        
        document.body.classList.toggle('simulation-paused', isPaused);
        
        // Stop any CSS animations if we had them
        const containers = document.querySelectorAll('.video-container');
        containers.forEach(c => {
            c.style.animationPlayState = isPaused ? 'paused' : 'running';
        });
    });

    // Panel Hover Feedback
    panelCards.forEach(card => {
        card.addEventListener('mouseenter', () => {
            if (!isPaused) {
                // In the real UI, this pauses the specific panel
                card.querySelector('.video-container').style.filter = 'brightness(1.1)';
            }
        });
        
        card.addEventListener('mouseleave', () => {
            card.querySelector('.video-container').style.filter = '';
        });
    });

    // Simulation: Subtle movement to represent "live" stream
    let tick = 0;
    function animateSimulation() {
        if (!isPaused) {
            tick += 0.05;
            // Shift the macroblock overlays slightly to simulate noise
            const overlays = document.querySelectorAll('.pixel-2m::after, .pixel-5m::after');
            overlays.forEach((ov, index) => {
                const shift = Math.sin(tick + index) * 2;
                ov.style.backgroundPosition = `${shift}px ${shift}px`;
            });
        }
        requestAnimationFrame(animateSimulation);
    }
    
    animateSimulation();
});
