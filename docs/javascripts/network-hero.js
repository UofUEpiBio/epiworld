(function () {
  "use strict";

  const HERO_SELECTOR = ".epiworld-network-hero";
  const CANVAS_SELECTOR = ".epiworld-network-canvas";

  const config = {
    particleCount: 150,
    connectionDistance: 150,
    speed: 0.2,
    particleColor: "rgba(255, 255, 255, 0.2)",
    lineOpacityScale: 0.1
  };

  let animationFrameId = null;
  let resizeObserver = null;
  let cleanupCurrent = null;

  function prefersReducedMotion() {
    return window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  }

  function startHeroAnimation() {
    if (cleanupCurrent) {
      cleanupCurrent();
      cleanupCurrent = null;
    }

    const hero = document.querySelector(HERO_SELECTOR);
    if (!hero) {
      return;
    }

    const canvas = hero.querySelector(CANVAS_SELECTOR);
    if (!canvas) {
      return;
    }

    const ctx = canvas.getContext("2d");
    if (!ctx) {
      return;
    }

    let particles = [];
    let w = 0;
    let h = 0;

    function resizeCanvas() {
      const ratio = Math.min(window.devicePixelRatio || 1, 2);
      w = hero.clientWidth;
      h = hero.clientHeight;

      canvas.width = Math.floor(w * ratio);
      canvas.height = Math.floor(h * ratio);
      canvas.style.width = w + "px";
      canvas.style.height = h + "px";

      ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    }

    class Particle {
      constructor() {
        this.x = Math.random() * w;
        this.y = Math.random() * h;
        this.vx = (Math.random() - 0.5) * config.speed * 2;
        this.vy = (Math.random() - 0.5) * config.speed * 2;
        this.radius = Math.random() * 2 + 1;
      }

      update() {
        this.x += this.vx;
        this.y += this.vy;

        if (this.x < 0 || this.x > w) {
          this.vx = -this.vx;
        }

        if (this.y < 0 || this.y > h) {
          this.vy = -this.vy;
        }
      }

      draw() {
        ctx.beginPath();
        ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
        ctx.fillStyle = config.particleColor;
        ctx.fill();
      }
    }

    function initParticles() {
      particles = [];
      for (let i = 0; i < config.particleCount; i += 1) {
        particles.push(new Particle());
      }
    }

    function drawConnections() {
      for (let i = 0; i < particles.length; i += 1) {
        for (let j = i + 1; j < particles.length; j += 1) {
          const dx = particles[i].x - particles[j].x;
          const dy = particles[i].y - particles[j].y;
          const distance = Math.hypot(dx, dy);

          if (distance < config.connectionDistance) {
            const opacity = 1 - distance / config.connectionDistance;
            ctx.beginPath();
            ctx.moveTo(particles[i].x, particles[i].y);
            ctx.lineTo(particles[j].x, particles[j].y);
            ctx.strokeStyle = "rgba(255, 255, 255, " + opacity * config.lineOpacityScale + ")";
            ctx.lineWidth = 1;
            ctx.stroke();
          }
        }
      }
    }

    function animate() {
      ctx.clearRect(0, 0, w, h);

      for (let i = 0; i < particles.length; i += 1) {
        particles[i].update();
        particles[i].draw();
      }

      drawConnections();
      animationFrameId = window.requestAnimationFrame(animate);
    }

    resizeCanvas();
    initParticles();

    if (!prefersReducedMotion()) {
      animate();
    } else {
      for (let i = 0; i < particles.length; i += 1) {
        particles[i].draw();
      }
      drawConnections();
    }

    const onResize = function () {
      resizeCanvas();
      initParticles();
    };

    window.addEventListener("resize", onResize);

    if (typeof ResizeObserver !== "undefined") {
      resizeObserver = new ResizeObserver(onResize);
      resizeObserver.observe(hero);
    }

    cleanupCurrent = function () {
      window.removeEventListener("resize", onResize);

      if (resizeObserver) {
        resizeObserver.disconnect();
        resizeObserver = null;
      }

      if (animationFrameId !== null) {
        window.cancelAnimationFrame(animationFrameId);
        animationFrameId = null;
      }
    };
  }

  if (typeof document$ !== "undefined" && document$.subscribe) {
    document$.subscribe(startHeroAnimation);
  } else {
    document.addEventListener("DOMContentLoaded", startHeroAnimation);
  }
})();
