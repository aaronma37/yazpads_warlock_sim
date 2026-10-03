/**
 * Three.js 3D Scene Visualizer
 * Renders an interactive 3D arena of 100 simulated configuration nodes
 * with dynamic height scaling, particle rings, color coding by spec,
 * and mouse raycasting for configuration inspection.
 */

import * as THREE from 'https://unpkg.com/three@0.160.0/build/three.module.js';

export class SceneVisualizer {
    constructor(canvasContainer, onSelectConfig) {
        this.container = canvasContainer;
        this.onSelectConfig = onSelectConfig;
        
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x0a0c14);
        this.scene.fog = new THREE.FogExp2(0x0a0c14, 0.015);

        this.camera = new THREE.PerspectiveCamera(
            45,
            this.container.clientWidth / this.container.clientHeight,
            0.1,
            1000
        );
        this.camera.position.set(0, 35, 45);

        this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
        this.renderer.setSize(this.container.clientWidth, this.container.clientHeight);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.container.appendChild(this.renderer.domElement);

        this.nodes = [];
        this.particleSystems = [];
        this.selectedNodeIndex = 0;
        this.hoveredNodeIndex = -1;

        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2(-999, -999);

        this._initLighting();
        this._initGround();
        this._initControls();
        this._initEventListeners();
        
        this.animate = this.animate.bind(this);
        requestAnimationFrame(this.animate);
    }

    _initLighting() {
        const ambientLight = new THREE.AmbientLight(0x2d3748, 1.2);
        this.scene.add(ambientLight);

        const dirLight = new THREE.DirectionalLight(0xffffff, 1.5);
        dirLight.position.set(20, 50, 20);
        dirLight.castShadow = true;
        dirLight.shadow.mapSize.width = 1024;
        dirLight.shadow.mapSize.height = 1024;
        this.scene.add(dirLight);

        const centerGlow = new THREE.PointLight(0x805ad5, 2.0, 50);
        centerGlow.position.set(0, 5, 0);
        this.scene.add(centerGlow);
    }

    _initGround() {
        const gridHelper = new THREE.GridHelper(60, 30, 0x4a5568, 0x1a202c);
        gridHelper.position.y = 0.01;
        this.scene.add(gridHelper);

        const groundGeo = new THREE.PlaneGeometry(120, 120);
        const groundMat = new THREE.MeshStandardMaterial({
            color: 0x07090e,
            roughness: 0.85,
            metalness: 0.2
        });
        const ground = new THREE.Mesh(groundGeo, groundMat);
        ground.rotation.x = -Math.PI / 2;
        ground.receiveShadow = true;
        this.scene.add(ground);
    }

    _initControls() {
        // Simple orbital drag control without external library dependency
        let isDragging = false;
        let prevMousePos = { x: 0, y: 0 };
        let spherical = { radius: 55, phi: Math.PI / 3.5, theta: 0 };

        const updateCamera = () => {
            this.camera.position.x = spherical.radius * Math.sin(spherical.phi) * Math.sin(spherical.theta);
            this.camera.position.y = spherical.radius * Math.cos(spherical.phi);
            this.camera.position.z = spherical.radius * Math.sin(spherical.phi) * Math.cos(spherical.theta);
            this.camera.lookAt(0, 2, 0);
        };

        this.renderer.domElement.addEventListener('mousedown', (e) => {
            isDragging = true;
            prevMousePos = { x: e.clientX, y: e.clientY };
        });

        window.addEventListener('mouseup', () => { isDragging = false; });

        window.addEventListener('mousemove', (e) => {
            const rect = this.renderer.domElement.getBoundingClientRect();
            this.mouse.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
            this.mouse.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

            if (isDragging) {
                const deltaX = e.clientX - prevMousePos.x;
                const deltaY = e.clientY - prevMousePos.y;
                spherical.theta -= deltaX * 0.008;
                spherical.phi = Math.max(0.1, Math.min(Math.PI / 2 - 0.05, spherical.phi - deltaY * 0.008));
                prevMousePos = { x: e.clientX, y: e.clientY };
                updateCamera();
            }
        });

        this.renderer.domElement.addEventListener('wheel', (e) => {
            e.preventDefault();
            spherical.radius = Math.max(15, Math.min(100, spherical.radius + e.deltaY * 0.05));
            updateCamera();
        }, { passive: false });

        updateCamera();
    }

    _initEventListeners() {
        window.addEventListener('resize', () => {
            if (!this.container) return;
            const w = this.container.clientWidth;
            const h = this.container.clientHeight;
            this.camera.aspect = w / h;
            this.camera.updateProjectionMatrix();
            this.renderer.setSize(w, h);
        });

        this.renderer.domElement.addEventListener('click', () => {
            if (this.hoveredNodeIndex >= 0) {
                this.selectedNodeIndex = this.hoveredNodeIndex;
                if (this.onSelectConfig) {
                    this.onSelectConfig(this.selectedNodeIndex);
                }
            }
        });
    }

    /**
     * Create 3D Nodes representing the 100 batch configurations
     */
    buildNodes(configs) {
        // Clear existing
        for (const node of this.nodes) {
            this.scene.remove(node.mesh);
            this.scene.remove(node.ring);
        }
        this.nodes = [];

        const gridSize = 10;
        const spacing = 3.6;
        const offset = (gridSize - 1) * spacing * 0.5;

        const specColors = {
            'Affliction': 0x9f7aea,     // Purple
            'Destruction': 0xed8936,    // Orange
            'SM/Ruin': 0x667eea,        // Indigo
            'Demonology': 0x48bb78,     // Fel Green
            'Speed': 0x38b2ac           // Cyan
        };

        const boxGeo = new THREE.BoxGeometry(1.8, 1, 1.8);
        const ringGeo = new THREE.RingGeometry(1.2, 1.4, 16);
        ringGeo.rotateX(-Math.PI / 2);

        for (let i = 0; i < configs.length; i++) {
            const row = Math.floor(i / gridSize);
            const col = i % gridSize;
            const posX = col * spacing - offset;
            const posZ = row * spacing - offset;

            const cfg = configs[i];
            let baseColor = 0x9f7aea;
            for (const [key, colHex] of Object.entries(specColors)) {
                if (cfg.spec.includes(key)) {
                    baseColor = colHex;
                    break;
                }
            }

            const mat = new THREE.MeshStandardMaterial({
                color: baseColor,
                roughness: 0.3,
                metalness: 0.6,
                emissive: baseColor,
                emissiveIntensity: 0.2
            });

            const mesh = new THREE.Mesh(boxGeo, mat);
            mesh.position.set(posX, 0.5, posZ);
            mesh.castShadow = true;
            mesh.receiveShadow = true;
            mesh.userData = { configIndex: i, baseColor, height: 1.0 };
            this.scene.add(mesh);

            const ringMat = new THREE.MeshBasicMaterial({
                color: baseColor,
                side: THREE.DoubleSide,
                transparent: true,
                opacity: 0.4
            });
            const ring = new THREE.Mesh(ringGeo, ringMat);
            ring.position.set(posX, 0.05, posZ);
            this.scene.add(ring);

            this.nodes.push({ mesh, ring, config: cfg, dps: 0 });
        }
    }

    /**
     * Update 3D visualization using GPU simulation results
     */
    updateResults(results) {
        let maxDps = 1;
        for (const r of results) {
            if (r.dps > maxDps) maxDps = r.dps;
        }

        for (let i = 0; i < results.length && i < this.nodes.length; i++) {
            const r = results[i];
            const node = this.nodes[i];
            node.dps = r.dps;

            const normalizedHeight = Math.max(0.5, (r.dps / maxDps) * 12.0);
            node.mesh.scale.y = normalizedHeight;
            node.mesh.position.y = normalizedHeight * 0.5;
            node.mesh.userData.height = normalizedHeight;

            // Highlight top performers
            if (r.dps > maxDps * 0.90) {
                node.mesh.material.emissiveIntensity = 0.8;
                node.ring.material.opacity = 0.9;
            } else {
                node.mesh.material.emissiveIntensity = 0.2;
                node.ring.material.opacity = 0.4;
            }
        }
    }

    animate() {
        requestAnimationFrame(this.animate);

        const time = performance.now() * 0.002;

        // Animate Rings & Selected Node Glow
        for (let i = 0; i < this.nodes.length; i++) {
            const node = this.nodes[i];
            if (i === this.selectedNodeIndex) {
                node.mesh.material.emissiveIntensity = 0.9 + 0.3 * Math.sin(time * 4);
                node.ring.rotation.y = time * 2;
                node.ring.scale.setScalar(1.2 + 0.1 * Math.sin(time * 4));
            } else if (i === this.hoveredNodeIndex) {
                node.mesh.material.emissiveIntensity = 0.6;
            } else {
                node.ring.rotation.y = time * 0.5;
            }
        }

        // Raycasting for interactive node hover
        this.raycaster.setFromCamera(this.mouse, this.camera);
        const intersects = this.raycaster.intersectObjects(this.nodes.map(n => n.mesh));

        if (intersects.length > 0) {
            const hit = intersects[0].object;
            this.hoveredNodeIndex = hit.userData.configIndex;
            document.body.style.cursor = 'pointer';
        } else {
            this.hoveredNodeIndex = -1;
            document.body.style.cursor = 'default';
        }

        this.renderer.render(this.scene, this.camera);
    }
}
